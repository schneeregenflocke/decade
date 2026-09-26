#include "calendar_layout.hpp"

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <utility>

#include "../../domain/calendar_config.hpp"
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

  RectF page_margin_area = fields.print_area;
  page_margin_area.SetTop(fields.title_area.Bottom());

  fields.calendar_area =
      page_margin_area.Reduce(RectF(kZero, kDefaultMargin, kZero, kZero));

  const TimelineProjection projection(calendar_config);
  const std::size_t band_count =
      kLabelAndLegendBands + calendar_config.YearCount();
  const float band_height =
      fields.calendar_area.Height() / static_cast<float>(band_count);
  const float label_and_legend_height =
      band_height * static_cast<float>(kLabelAndLegendBands);
  const float row_labels_width =
      fields.calendar_area.Width() / kCalendarColumns;

  fields.cells_area = fields.calendar_area.Reduce(
      RectF(row_labels_width, kZero, label_and_legend_height, kZero));

  fields.proportions.SetupRowAreas(fields.cells_area, projection.RowCount());
  const BandProportions band_proportions =
      calendar_config.LaidOutBandProportions();
  fields.proportions.SetupSubAreas(band_proportions);

  fields.band_proportions.SetupRowAreas(RectF(kZero, kZero, kZero, band_height),
                                        1);
  fields.band_proportions.SetupSubAreas(band_proportions);

  fields.day_width =
      fields.cells_area.Width() / static_cast<float>(projection.RowDays());

  fields.x_labels_area = fields.calendar_area.Reduce(
      RectF(row_labels_width, kZero, band_height, fields.cells_area.Height()));
  fields.y_labels_area = fields.calendar_area.Reduce(
      RectF(kZero, fields.cells_area.Width(), label_and_legend_height, kZero));
  fields.legend_area = fields.calendar_area.Reduce(
      RectF(row_labels_width, kZero, kZero,
            fields.cells_area.Height() + band_height));

  return fields;
}
