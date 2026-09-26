#include "grid_sections.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#include "../../domain/band_part.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_period.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "calendar_scene_nodes.hpp"
#include "section_context.hpp"

namespace calendar_sections {

namespace {

constexpr int kDaysPerWeek = 7;

std::vector<std::string> MonthNames() {
  constexpr std::size_t kMonthCount = 12;
  std::array<char, detail::kMonthNameBufferSize> buf{};
  std::vector<std::string> names(kMonthCount);
  for (std::size_t index = 0; index < kMonthCount; ++index) {
    std::tm month_tm = {};
    month_tm.tm_mon = static_cast<int>(index);
    if (std::strftime(buf.data(), std::size(buf), "%b", &month_tm) != 0) {
      names[index] = buf.data();
    }
  }
  return names;
}

// One per column of the first row; every row shares them.
std::vector<std::string> ColumnLabels(const SectionContext& ctx) {
  switch (ctx.projection.Columns()) {
    case ColumnUnit::kMonth:
      return MonthNames();
    case ColumnUnit::kYear: {
      std::vector<std::string> years;
      for (const DatePeriod& year :
           SplitAtYearBoundaries(ctx.projection.RowPeriod(0))) {
        years.push_back(std::to_string(year.Begin().Year()));
      }
      return years;
    }
  }
  return {};
}

// The bounds of a row's period as years: one year alone, or first and last.
std::string RowLabel(const DatePeriod& period) {
  const int first_year = period.Begin().Year();
  const int last_year = period.Last().Year();
  if (first_year == last_year) {
    return std::to_string(first_year);
  }
  return std::to_string(first_year) + "\u2013" + std::to_string(last_year);
}

std::vector<RectF> EqualColumns(const RectF& area, std::size_t count) {
  std::vector<RectF> columns(count, area);
  const float width = area.Width() / static_cast<float>(count);
  for (std::size_t index = 0; index < count; ++index) {
    const float left = area.Left() + (width * static_cast<float>(index));
    columns[index].SetLeft(left);
    columns[index].SetRight(left + width);
  }
  return columns;
}

}  // namespace

void BuildCalendarLabels(const SectionContext& ctx) {
  // Column and row labels carry the application-wide chosen size in points —
  // they label the page, not the individual bar, and should therefore not
  // travel with the cell size.
  const float labels_font_size = ctx.font_config.SizeMillimetres();

  const std::vector<std::string> column_labels = ColumnLabels(ctx);
  const std::vector<RectF> x_label_frames =
      EqualColumns(ctx.layout.XLabelsArea(), column_labels.size());
  auto column_label_texts = detail::TextPool(ctx, ctx.nodes.column_label_texts);
  for (std::size_t index = 0; index < column_labels.size(); ++index) {
    detail::SetCenteredText(ctx, column_label_texts, column_labels[index],
                            column_labels[index],
                            x_label_frames[index].Center(), labels_font_size);
  }

  const auto config = ctx.shape_config.GetShapeConfiguration(
      ShapeConfigSet::kCalendarLabelsKey);
  detail::FillRectangles(ctx.nodes.column_labels, x_label_frames, config);

  const std::size_t row_count = ctx.projection.RowCount();
  auto row_label_texts = detail::TextPool(ctx, ctx.nodes.row_label_texts);
  std::vector<RectF> y_label_frames(row_count);
  for (std::size_t row = 0; row < row_count; ++row) {
    const std::string text = RowLabel(ctx.projection.RowPeriod(row));
    RectF& frame = y_label_frames.at(row);
    frame = ctx.layout.GetRowArea(row);
    frame.SetLeft(ctx.layout.YLabelsArea().Left());
    frame.SetRight(ctx.layout.YLabelsArea().Right());
    detail::SetCenteredText(ctx, row_label_texts, text, text, frame.Center(),
                            labels_font_size);
  }

  detail::FillRectangles(ctx.nodes.row_labels, y_label_frames, config);
}

void BuildYears(const SectionContext& ctx) {
  std::vector<RectF> year_cells;
  for (const DatePeriod& year :
       SplitAtYearBoundaries(ctx.calendar_config.Period())) {
    year_cells.push_back(detail::PeriodArea(ctx, year, BandPart::kDays));
  }

  detail::FillRectangles(
      ctx.nodes.year_cells, year_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kYearsShapesKey));
}

void BuildMonths(const SectionContext& ctx) {
  const DatePeriod& span = ctx.calendar_config.Period();
  std::vector<RectF> month_cells;
  for (Date month = span.Begin(); month < span.End();
       month = month.AddMonths(1)) {
    month_cells.push_back(detail::PeriodArea(
        ctx, DatePeriod(month, month.AddMonths(1)), BandPart::kDays));
  }

  detail::FillRectangles(
      ctx.nodes.month_cells, month_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kMonthsShapesKey));
}

// Row by row rather than through PeriodArea: a weekday that steps on its own
// spares every day of the span a trip through the calendar backend. Under
// year columns a day is too narrow to draw, so the months are the finest cell.
void BuildDays(const SectionContext& ctx) {
  std::vector<RectF> day_cells;
  std::vector<RectF> sunday_cells;

  const std::size_t drawn_rows = ctx.projection.Columns() == ColumnUnit::kMonth
                                     ? ctx.projection.RowCount()
                                     : 0;
  for (std::size_t row = 0; row < drawn_rows; ++row) {
    const DatePeriod row_period = ctx.projection.RowPeriod(row);
    const RectF row_area = ctx.layout.GetPartArea(row, BandPart::kDays);
    const auto first_weekday =
        static_cast<std::int64_t>(row_period.Begin().DayOfWeek());

    for (std::int64_t day = 0; day < row_period.LengthDays(); ++day) {
      RectF day_cell = row_area;
      day_cell.SetLeft(row_area.Left() +
                       (static_cast<float>(day) * ctx.layout.DayWidth()));
      day_cell.SetRight(day_cell.Left() + ctx.layout.DayWidth());
      const bool is_sunday = (first_weekday + day) % kDaysPerWeek ==
                             static_cast<std::int64_t>(Weekday::kSunday);
      (is_sunday ? sunday_cells : day_cells).push_back(day_cell);
    }
  }

  detail::FillRectangles(
      ctx.nodes.day_cells, day_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kDayShapesKey));
  detail::FillRectangles(
      ctx.nodes.sunday_cells, sunday_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kSundayShapesKey));
}

}  // namespace calendar_sections
