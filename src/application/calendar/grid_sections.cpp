#include "grid_sections.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

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
  constexpr size_t number_months = 12;
  std::array<char, detail::kMonthNameBufferSize> buf{};
  constexpr const char* format = "%b";
  std::array<std::string, number_months> months_names;

  for (size_t index = 0; index < months_names.size(); ++index) {
    std::tm month_tm = {};
    month_tm.tm_mon = static_cast<int>(index);

    if (std::strftime(buf.data(), std::size(buf), format, &month_tm) != 0) {
      months_names.at(index) = buf.data();
    }
  }

  // Month names and year numbers carry the application-wide chosen size in
  // points — they label the page, not the individual bar, and should therefore
  // not travel with the cell size.
  const float labels_font_size = ctx.font_config.SizeMillimetres();

  const std::vector<RectF> x_label_frames =
      EqualColumns(ctx.layout.XLabelsArea(), number_months);
  auto month_labels = detail::TextPool(ctx, ctx.nodes.month_labels);
  for (size_t index = 0; index < number_months; ++index) {
    detail::SetCenteredText(
        ctx, month_labels, months_names.at(index), months_names.at(index),
        x_label_frames.at(index).Center(), labels_font_size);
  }

  const auto config = ctx.shape_config.GetShapeConfiguration(
      ShapeConfigSet::kCalendarLabelsKey);
  detail::FillRectangles(ctx.nodes.column_labels, x_label_frames, config);

  const std::size_t row_count = ctx.projection.RowCount();
  auto row_labels = detail::TextPool(ctx, ctx.nodes.year_labels);
  std::vector<RectF> y_label_frames(row_count);
  for (std::size_t row = 0; row < row_count; ++row) {
    const std::string text =
        std::to_string(ctx.projection.RowPeriod(row).Begin().Year());
    RectF& frame = y_label_frames.at(row);
    frame = ctx.layout.GetRowArea(row);
    frame.SetLeft(ctx.layout.YLabelsArea().Left());
    frame.SetRight(ctx.layout.YLabelsArea().Right());
    detail::SetCenteredText(ctx, row_labels, text, text, frame.Center(),
                            labels_font_size);
  }

  detail::FillRectangles(ctx.nodes.row_labels, y_label_frames, config);
}

void BuildYears(const SectionContext& ctx) {
  std::vector<RectF> year_cells;
  for (const DatePeriod& year :
       SplitAtYearBoundaries(ctx.calendar_config.Period())) {
    year_cells.push_back(detail::PeriodArea(ctx, year, 1));
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
    month_cells.push_back(
        detail::PeriodArea(ctx, DatePeriod(month, month.AddMonths(1)), 1));
  }

  detail::FillRectangles(
      ctx.nodes.month_cells, month_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kMonthsShapesKey));
}

// Row by row rather than through PeriodArea: a weekday that steps on its own
// spares every day of the span a trip through the calendar backend.
void BuildDays(const SectionContext& ctx) {
  std::vector<RectF> day_cells;
  std::vector<RectF> sunday_cells;

  for (std::size_t row = 0; row < ctx.projection.RowCount(); ++row) {
    const DatePeriod row_period = ctx.projection.RowPeriod(row);
    const RectF row_area = ctx.layout.GetSubArea(row, 1);
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
