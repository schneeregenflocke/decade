#include "calendar_layout.hpp"

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <utility>

#include "../../domain/calendar_config.hpp"
#include "../../domain/calendar_sizing.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../infrastructure/graphics/rect.hpp"

CalendarLayout::CalendarLayout(const RectF& page_size, const RectF& page_margin,
                               float title_area_height,
                               const CalendarConfig& calendar_config)
    : fields_(Compute(page_size, page_margin, title_area_height,
                      calendar_config)) {}

const glm::vec3& CalendarLayout::PrintAreaOrigin() const {
  return fields_.print_area_origin;
}

const RectF& CalendarLayout::PrintArea() const { return fields_.print_area; }

const RectF& CalendarLayout::TitleArea() const { return fields_.title_area; }

const RectF& CalendarLayout::CalendarArea() const {
  return fields_.calendar_area;
}

const RectF& CalendarLayout::CellsArea() const { return fields_.cells_area; }

const RectF& CalendarLayout::XLabelsArea() const {
  return fields_.x_labels_area;
}

const RectF& CalendarLayout::YLabelsArea() const {
  return fields_.y_labels_area;
}

const RectF& CalendarLayout::LegendArea() const { return fields_.legend_area; }

float CalendarLayout::DayWidth() const { return fields_.day_width; }

RectF CalendarLayout::GetRowArea(std::size_t row) const {
  return fields_.proportions.GetRowArea(row);
}

RectF CalendarLayout::GetPartArea(std::size_t row, BandPart part) const {
  return fields_.proportions.GetSubArea(row, ContentIndex(part));
}

float CalendarLayout::BandPartHeight(BandPart part) const {
  return fields_.band_proportions.GetSubArea(0, ContentIndex(part)).Height();
}

std::size_t CalendarLayout::ContentIndex(BandPart part) {
  return std::to_underlying(part) / 2;
}

CalendarLayout::Heights CalendarLayout::ComputeHeights(
    const RectF& available, const CalendarConfig& config) {
  const CalendarSizing& sizing = config.Sizing();
  if (sizing.FixesHeight()) {
    const CalendarSizing::Heights& fixed = sizing.FixedHeights();
    return {.band = fixed.band,
            .column_labels = fixed.column_labels,
            .legend = fixed.legend};
  }
  const std::size_t band_count = kLabelAndLegendBands + config.YearCount();
  const float band = available.Height() / static_cast<float>(band_count);
  return {.band = band, .column_labels = band, .legend = band};
}

CalendarLayout::Widths CalendarLayout::ComputeWidths(
    const RectF& available, const CalendarConfig& config,
    std::int64_t row_days) {
  const CalendarSizing& sizing = config.Sizing();
  if (sizing.FixesWidth()) {
    const CalendarSizing::Widths& fixed = sizing.FixedWidths();
    return {.row_labels = fixed.row_labels,
            .cells = fixed.day * static_cast<float>(row_days)};
  }
  const float calendar_width = available.Width() - kDefaultMargin;
  const float row_labels = calendar_width / kCalendarColumns;
  return {.row_labels = row_labels, .cells = calendar_width - row_labels};
}

float CalendarLayout::LegendEntryWidth(std::size_t entry_count) const {
  return fields_.fixed_legend_entry_width.value_or(
      fields_.legend_area.Width() / static_cast<float>(entry_count));
}

CalendarLayout::Fields CalendarLayout::Compute(
    const RectF& page_size, const RectF& page_margin, float title_area_height,
    const CalendarConfig& calendar_config) {
  Fields fields;

  // The print area is the page minus the margins, then shifted so its
  // bottom-left is the local origin; print_area_origin carries that offset so
  // the caller can position the print-area node and the bars' pick boxes.
  fields.print_area = page_size.Reduce(page_margin);
  fields.print_area_origin = fields.print_area.LeftBottom();
  fields.print_area = fields.print_area.Shift(-fields.print_area_origin.x,
                                              -fields.print_area_origin.y);

  fields.title_area = fields.print_area;
  fields.title_area.SetBottom(fields.title_area.Top() - title_area_height);

  RectF below_title = fields.print_area;
  below_title.SetTop(fields.title_area.Bottom());

  const TimelineProjection projection(calendar_config);
  const Heights heights = ComputeHeights(below_title, calendar_config);
  const Widths widths =
      ComputeWidths(below_title, calendar_config, projection.RowDays());
  const float cells_height =
      heights.band * static_cast<float>(calendar_config.YearCount());

  fields.calendar_area = RectF(
      below_title.Left(), below_title.Left() + widths.row_labels + widths.cells,
      below_title.Top() - cells_height - heights.column_labels - heights.legend,
      below_title.Top());

  fields.cells_area = fields.calendar_area.Reduce(RectF(
      widths.row_labels, kZero, heights.column_labels + heights.legend, kZero));

  fields.proportions.SetupRowAreas(fields.cells_area, projection.RowCount());
  const BandProportions band_proportions =
      calendar_config.LaidOutBandProportions();
  fields.proportions.SetupSubAreas(band_proportions);

  fields.band_proportions.SetupRowAreas(
      RectF(kZero, kZero, kZero, heights.band), 1);
  fields.band_proportions.SetupSubAreas(band_proportions);

  fields.day_width = widths.cells / static_cast<float>(projection.RowDays());

  fields.x_labels_area = fields.calendar_area.Reduce(
      RectF(widths.row_labels, kZero, heights.legend, cells_height));
  fields.y_labels_area = fields.calendar_area.Reduce(RectF(
      kZero, widths.cells, heights.column_labels + heights.legend, kZero));
  fields.legend_area = fields.calendar_area.Reduce(RectF(
      widths.row_labels, kZero, kZero, cells_height + heights.column_labels));

  if (calendar_config.Sizing().FixesWidth()) {
    fields.fixed_legend_entry_width =
        calendar_config.Sizing().FixedWidths().legend_entry;
  }

  return fields;
}
