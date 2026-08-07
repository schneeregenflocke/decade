#ifndef GRID_SECTIONS_HPP
#define GRID_SECTIONS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#include "../../domain/date.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "section_context.hpp"

// The calendar grid: the month and year labels around it and the year, month
// and day cells inside it.

namespace calendar_sections {

inline void BuildCalendarLabels(const SectionContext& ctx) {
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

  std::vector<RectF> x_label_frames(number_months);
  // Month names and year numbers carry the application-wide chosen size in
  // points — they label the page, not the individual bar, and should therefore
  // not travel with the cell size.
  const float labels_font_size = ctx.font_config.SizeMillimetres();

  const auto& month_node = ctx.nodes.month_labels;

  month_node->RemoveChildren();
  for (size_t index = 0; index < number_months; ++index) {
    const auto float_index = static_cast<float>(index);
    const auto left = ctx.layout.XLabelsArea().Left() +
                      (ctx.layout.CellWidth() * float_index);
    x_label_frames.at(index).SetLeft(left);
    x_label_frames.at(index).SetRight(left + ctx.layout.CellWidth());
    x_label_frames.at(index).SetBottom(ctx.layout.XLabelsArea().Bottom());
    x_label_frames.at(index).SetTop(ctx.layout.XLabelsArea().Top());

    detail::AddCenteredText(
        ctx, month_node, months_names.at(index), months_names.at(index),
        x_label_frames.at(index).Center(), labels_font_size);
  }

  const auto config =
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kCalendarLabels);
  detail::FillRectangles(ctx.nodes.column_labels, x_label_frames, config);

  const auto& year_node = ctx.nodes.year_labels;

  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  if (span_years == 0) {
    return;
  }

  const TimelineProjection projection(ctx.calendar_config);
  std::vector<RectF> y_labels_frames(span_years);
  year_node->RemoveChildren();
  for (std::size_t index = 0; index < span_years; ++index) {
    const std::string current_year_text =
        std::to_string(projection.YearForRow(index));

    const auto float_index = static_cast<float>(index);
    const auto bottom = ctx.layout.YLabelsArea().Bottom() +
                        (ctx.layout.RowHeight() * float_index);
    y_labels_frames.at(index).SetLeft(ctx.layout.YLabelsArea().Left());
    y_labels_frames.at(index).SetRight(ctx.layout.YLabelsArea().Right());
    y_labels_frames.at(index).SetBottom(bottom);
    y_labels_frames.at(index).SetTop(bottom + ctx.layout.RowHeight());

    detail::AddCenteredText(
        ctx, year_node, current_year_text, current_year_text,
        y_labels_frames.at(index).Center(), labels_font_size);
  }

  detail::FillRectangles(ctx.nodes.row_labels, y_labels_frames, config);
}

inline void BuildYears(const SectionContext& ctx) {
  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  if (span_years == 0) {
    return;
  }

  const TimelineProjection projection(ctx.calendar_config);
  std::vector<RectF> year_cells(span_years);

  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year = projection.YearForRow(index);
    const auto number_days = DaysInYear(current_year);
    const float year_length =
        static_cast<float>(number_days) * ctx.layout.DayWidth();
    RectF year_cell = ctx.layout.GetSubArea(index, 1);
    year_cell.SetRight(year_cell.Left() + year_length);
    year_cells.at(index) = year_cell;
  }

  detail::FillRectangles(
      ctx.nodes.year_cells, year_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kYearsShapes));
}

inline void BuildMonths(const SectionContext& ctx) {
  constexpr size_t number_months = 12;
  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  if (span_years == 0) {
    return;
  }

  const auto store_size = number_months * span_years;
  const TimelineProjection projection(ctx.calendar_config);
  std::vector<RectF> month_cells(store_size);

  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year = projection.YearForRow(index);
    const Date first_day_of_year = Date::FromYmd(current_year, 1, 1);

    for (size_t subindex = 0; subindex < number_months; ++subindex) {
      const auto current_cell = ctx.layout.GetSubArea(index, 1);
      const int month_index = static_cast<int>(subindex);
      RectF month_cell;
      const auto start_offset =
          static_cast<float>(Date::DaysBetween(
              first_day_of_year, first_day_of_year.AddMonths(month_index))) *
          ctx.layout.DayWidth();
      const auto end_offset =
          static_cast<float>(
              Date::DaysBetween(first_day_of_year,
                                first_day_of_year.AddMonths(month_index + 1))) *
          ctx.layout.DayWidth();
      month_cell.SetLeft(current_cell.Left() + start_offset);
      month_cell.SetRight(current_cell.Left() + end_offset);
      month_cell.SetBottom(current_cell.Bottom());
      month_cell.SetTop(current_cell.Top());

      const auto store_index = (index * number_months) + subindex;
      month_cells.at(store_index) = month_cell;
    }
  }

  detail::FillRectangles(
      ctx.nodes.month_cells, month_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kMonthsShapes));
}

inline void BuildDays(const SectionContext& ctx) {
  if (!ctx.calendar_config.IsValidSpan()) {
    return;
  }

  const auto span_days = ctx.calendar_config.GetSpanLengthDays();
  if (span_days <= 0) {
    return;
  }

  std::int64_t days_index = 0;
  const auto number_days_cells = static_cast<size_t>(span_days);

  std::vector<RectF> day_cells;
  std::vector<RectF> sunday_cells;
  day_cells.resize(number_days_cells);
  sunday_cells.resize(number_days_cells);

  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  const TimelineProjection projection(ctx.calendar_config);
  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year = projection.YearForRow(index);
    const std::int64_t number_days = DaysInYear(current_year);

    for (std::int64_t subindex = 0; subindex < number_days; ++subindex) {
      const auto float_subindex = static_cast<float>(subindex);
      const auto current_cell = ctx.layout.GetSubArea(index, 1);

      const Date current_date =
          ctx.calendar_config.GetSpanLimitsDate().at(0).AddDays(
              static_cast<int>(days_index));

      if (current_date.DayOfWeek() == Weekday::kSunday) {
        RectF day_cell;
        day_cell.SetLeft(current_cell.Left() +
                         (float_subindex * ctx.layout.DayWidth()));
        day_cell.SetRight(day_cell.Left() + ctx.layout.DayWidth());
        day_cell.SetBottom(current_cell.Bottom());
        day_cell.SetTop(current_cell.Top());
        sunday_cells[static_cast<size_t>(days_index)] = day_cell;
      } else {
        RectF day_cell;
        day_cell.SetLeft(current_cell.Left() +
                         (float_subindex * ctx.layout.DayWidth()));
        day_cell.SetRight(day_cell.Left() + ctx.layout.DayWidth());
        day_cell.SetBottom(current_cell.Bottom());
        day_cell.SetTop(current_cell.Top());
        day_cells[static_cast<size_t>(days_index)] = day_cell;
      }
      ++days_index;
    }
  }

  detail::FillRectangles(
      ctx.nodes.day_cells, day_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kDayShapes));
  detail::FillRectangles(
      ctx.nodes.sunday_cells, sunday_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kSundayShapes));
}

}  // namespace calendar_sections

#endif  // GRID_SECTIONS_HPP
