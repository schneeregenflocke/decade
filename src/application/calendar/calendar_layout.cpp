#include "calendar_layout.hpp"

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "../../infrastructure/graphics/rect.hpp"

CalendarLayout::CalendarLayout(const RectF& page_size, const RectF& page_margin,
                               float title_area_height,
                               std::size_t span_length_years,
                               const std::vector<float>& spacing_proportions)
    : fields_(Compute(page_size, page_margin, title_area_height,
                      span_length_years, spacing_proportions)) {}

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

float CalendarLayout::CellWidth() const { return fields_.cell_width; }

float CalendarLayout::RowHeight() const { return fields_.row_height; }

float CalendarLayout::DayWidth() const { return fields_.day_width; }

RectF CalendarLayout::GetSubArea(std::size_t row, std::size_t sub) const {
  return fields_.proportions.GetSubArea(row, sub);
}

CalendarLayout::Fields CalendarLayout::Compute(
    const RectF& page_size, const RectF& page_margin, float title_area_height,
    std::size_t span_length_years,
    const std::vector<float>& spacing_proportions) {
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

  const std::size_t number_rows = kAdditionalRows + span_length_years;
  fields.cell_width = fields.calendar_area.Width() / kCalendarColumns;
  fields.row_height =
      fields.calendar_area.Height() / static_cast<float>(number_rows);

  fields.cells_area = fields.calendar_area.Reduce(RectF(
      fields.cell_width, kZero, fields.row_height * kRowHeaderScale, kZero));

  fields.proportions.SetupRowAreas(fields.cells_area, span_length_years);
  fields.proportions.SetupSubAreas(spacing_proportions);

  fields.day_width = fields.cells_area.Width() / kDaysPerYear;

  fields.x_labels_area = fields.calendar_area.Reduce(RectF(
      fields.cell_width, kZero, fields.row_height, fields.cells_area.Height()));
  fields.y_labels_area = fields.calendar_area.Reduce(
      RectF(kZero, fields.cells_area.Width(),
            fields.row_height * kRowHeaderScale, kZero));
  fields.legend_area = fields.calendar_area.Reduce(
      RectF(fields.cell_width, kZero, kZero,
            fields.cells_area.Height() + fields.row_height));

  return fields;
}
