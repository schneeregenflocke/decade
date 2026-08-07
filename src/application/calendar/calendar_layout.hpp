#ifndef CALENDAR_LAYOUT_HPP
#define CALENDAR_LAYOUT_HPP

#include <cstddef>
#include <glm/vec3.hpp>
#include <vector>

#include "../../infrastructure/graphics/area_layout.hpp"
#include "../../infrastructure/graphics/rect.hpp"

// Application/Infrastructure bridge: the calendar's page geometry, computed
// once per rebuild from the page size, margins, title height and calendar span.
// It is pure (GL-free, wx-free) and depends only on primitives, so the whole
// layout is unit-testable without a GL context — which the previous inline
// computation inside CalendarSceneComposer::Build() was not.
//
// The areas are deliberately interdependent and therefore computed in one
// pass: print area -> title -> calendar -> cells -> row/sub proportions ->
// label/legend areas. Consumers (the section builders) read the results
// through the accessors; nobody recomputes geometry.
class CalendarLayout {
 public:
  CalendarLayout() = default;

  CalendarLayout(const RectF& page_size, const RectF& page_margin,
                 float title_area_height, std::size_t span_length_years,
                 const std::vector<float>& spacing_proportions)
      : fields_(Compute(page_size, page_margin, title_area_height,
                        span_length_years, spacing_proportions)) {}

  [[nodiscard]] const glm::vec3& PrintAreaOrigin() const {
    return fields_.print_area_origin;
  }
  [[nodiscard]] const RectF& PrintArea() const { return fields_.print_area; }
  [[nodiscard]] const RectF& TitleArea() const { return fields_.title_area; }
  [[nodiscard]] const RectF& CalendarArea() const {
    return fields_.calendar_area;
  }
  [[nodiscard]] const RectF& CellsArea() const { return fields_.cells_area; }
  [[nodiscard]] const RectF& XLabelsArea() const {
    return fields_.x_labels_area;
  }
  [[nodiscard]] const RectF& YLabelsArea() const {
    return fields_.y_labels_area;
  }
  [[nodiscard]] const RectF& LegendArea() const { return fields_.legend_area; }
  [[nodiscard]] float CellWidth() const { return fields_.cell_width; }
  [[nodiscard]] float RowHeight() const { return fields_.row_height; }
  [[nodiscard]] float DayWidth() const { return fields_.day_width; }

  // Sub-area of the given row/sub band from the proportional row layout.
  [[nodiscard]] RectF GetSubArea(std::size_t row, std::size_t sub) const {
    return fields_.proportions.GetSubArea(row, sub);
  }

 private:
  static constexpr float kZero = 0.0F;
  static constexpr float kDefaultMargin = 5.0F;
  static constexpr float kCalendarColumns = 13.0F;
  static constexpr float kRowHeaderScale = 2.0F;
  static constexpr float kDaysPerYear = 366.0F;
  static constexpr std::size_t kAdditionalRows = 2;

  // All computed geometry in one aggregate, so the constructor can initialise
  // it from a single pure function (rather than assigning members in its body).
  struct Fields {
    ProportionAreaLayout proportions;
    glm::vec3 print_area_origin{0.0F};
    RectF print_area;
    RectF title_area;
    RectF calendar_area;
    RectF cells_area;
    RectF x_labels_area;
    RectF y_labels_area;
    RectF legend_area;
    float cell_width{0.0F};
    float row_height{0.0F};
    float day_width{0.0F};
  };

  static Fields Compute(const RectF& page_size, const RectF& page_margin,
                        float title_area_height, std::size_t span_length_years,
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

    fields.x_labels_area = fields.calendar_area.Reduce(
        RectF(fields.cell_width, kZero, fields.row_height,
              fields.cells_area.Height()));
    fields.y_labels_area = fields.calendar_area.Reduce(
        RectF(kZero, fields.cells_area.Width(),
              fields.row_height * kRowHeaderScale, kZero));
    fields.legend_area = fields.calendar_area.Reduce(
        RectF(fields.cell_width, kZero, kZero,
              fields.cells_area.Height() + fields.row_height));

    return fields;
  }

  Fields fields_;
};

#endif  // CALENDAR_LAYOUT_HPP
