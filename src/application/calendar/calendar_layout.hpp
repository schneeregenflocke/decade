#ifndef CALENDAR_LAYOUT_HPP
#define CALENDAR_LAYOUT_HPP

#include <cstddef>
#include <glm/vec3.hpp>
#include <vector>

#include "../../infrastructure/graphics/area_layout.hpp"
#include "../../infrastructure/graphics/rect.hpp"

// Application/Infrastructure bridge: the calendar's page geometry, computed
// once per rebuild from the page size, margins, title height and calendar span.
// It is pure (GL-free, toolkit-free) and depends only on primitives, so the
// whole layout is unit-testable without a GL context — which the previous
// inline computation inside CalendarSceneComposer::Build() was not.
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
                 const std::vector<float>& spacing_proportions);

  [[nodiscard]] const glm::vec3& PrintAreaOrigin() const;
  [[nodiscard]] const RectF& PrintArea() const;
  [[nodiscard]] const RectF& TitleArea() const;
  [[nodiscard]] const RectF& CalendarArea() const;
  [[nodiscard]] const RectF& CellsArea() const;
  [[nodiscard]] const RectF& XLabelsArea() const;
  [[nodiscard]] const RectF& YLabelsArea() const;
  [[nodiscard]] const RectF& LegendArea() const;
  [[nodiscard]] float CellWidth() const;
  [[nodiscard]] float RowHeight() const;
  [[nodiscard]] float DayWidth() const;

  // Sub-area of the given row/sub band from the proportional row layout.
  [[nodiscard]] RectF GetSubArea(std::size_t row, std::size_t sub) const;

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
                        const std::vector<float>& spacing_proportions);

  Fields fields_;
};

#endif  // CALENDAR_LAYOUT_HPP
