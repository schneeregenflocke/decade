#ifndef CALENDAR_LAYOUT_HPP
#define CALENDAR_LAYOUT_HPP

#include <cstddef>
#include <glm/vec3.hpp>
#include <vector>

#include "../../infrastructure/graphics/frame_layout.hpp"
#include "../../infrastructure/graphics/rect.hpp"

// Application/Infrastructure bridge: the calendar's page geometry, computed
// once per rebuild from the page size, margins, title height and calendar span.
// It is pure (GL-free, wx-free) and depends only on primitives, so the whole
// layout is unit-testable without a GL context — which the previous inline
// computation inside CalendarSceneComposer::Build() was not.
//
// The frames are deliberately interdependent and therefore computed in one
// pass: print area -> title -> calendar -> cells -> row/sub proportions ->
// label/legend frames. Consumers (the section builders) read the results
// through the accessors; nobody recomputes geometry.
class CalendarLayout {
 public:
  CalendarLayout() = default;

  CalendarLayout(const RectF& page_size, const RectF& page_margin,
                 float title_frame_height, std::size_t span_length_years,
                 const std::vector<float>& spacing_proportions)
      : fields_(Compute(page_size, page_margin, title_frame_height,
                        span_length_years, spacing_proportions)) {}

  [[nodiscard]] const glm::vec3& PrintAreaOrigin() const {
    return fields_.print_area_origin;
  }
  [[nodiscard]] const RectF& PrintArea() const { return fields_.print_area; }
  [[nodiscard]] const RectF& TitleFrame() const { return fields_.title_frame; }
  [[nodiscard]] const RectF& CalendarFrame() const {
    return fields_.calendar_frame;
  }
  [[nodiscard]] const RectF& CellsFrame() const { return fields_.cells_frame; }
  [[nodiscard]] const RectF& XLabelsFrame() const {
    return fields_.x_labels_frame;
  }
  [[nodiscard]] const RectF& YLabelsFrame() const {
    return fields_.y_labels_frame;
  }
  [[nodiscard]] const RectF& LegendFrame() const {
    return fields_.legend_frame;
  }
  [[nodiscard]] float CellWidth() const { return fields_.cell_width; }
  [[nodiscard]] float RowHeight() const { return fields_.row_height; }
  [[nodiscard]] float DayWidth() const { return fields_.day_width; }

  // Sub-frame of the given row/sub band from the proportional row layout.
  [[nodiscard]] RectF GetSubFrame(std::size_t row, std::size_t sub) const {
    return fields_.proportions.GetSubFrame(row, sub);
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
    ProportionFrameLayout proportions;
    glm::vec3 print_area_origin{0.0F};
    RectF print_area;
    RectF title_frame;
    RectF calendar_frame;
    RectF cells_frame;
    RectF x_labels_frame;
    RectF y_labels_frame;
    RectF legend_frame;
    float cell_width{0.0F};
    float row_height{0.0F};
    float day_width{0.0F};
  };

  static Fields Compute(const RectF& page_size, const RectF& page_margin,
                        float title_frame_height, std::size_t span_length_years,
                        const std::vector<float>& spacing_proportions) {
    Fields fields;

    // The print area is the page minus the margins, then shifted so its
    // bottom-left is the local origin; print_area_origin carries that offset so
    // the caller can position the print-area node and the bars' pick boxes.
    fields.print_area = page_size.Reduce(page_margin);
    fields.print_area_origin = fields.print_area.LeftBottom();
    fields.print_area = fields.print_area.Shift(-fields.print_area_origin.x,
                                                -fields.print_area_origin.y);

    fields.title_frame = fields.print_area;
    fields.title_frame.SetBottom(fields.title_frame.Top() - title_frame_height);

    RectF page_margin_frame = fields.print_area;
    page_margin_frame.SetTop(fields.title_frame.Bottom());

    fields.calendar_frame =
        page_margin_frame.Reduce(RectF(kZero, kDefaultMargin, kZero, kZero));

    const std::size_t number_rows = kAdditionalRows + span_length_years;
    fields.cell_width = fields.calendar_frame.Width() / kCalendarColumns;
    fields.row_height =
        fields.calendar_frame.Height() / static_cast<float>(number_rows);

    fields.cells_frame = fields.calendar_frame.Reduce(RectF(
        fields.cell_width, kZero, fields.row_height * kRowHeaderScale, kZero));

    fields.proportions.SetupRowFrames(fields.cells_frame, span_length_years);
    fields.proportions.SetupSubFrames(spacing_proportions);

    fields.day_width = fields.cells_frame.Width() / kDaysPerYear;

    fields.x_labels_frame = fields.calendar_frame.Reduce(
        RectF(fields.cell_width, kZero, fields.row_height,
              fields.cells_frame.Height()));
    fields.y_labels_frame = fields.calendar_frame.Reduce(
        RectF(kZero, fields.cells_frame.Width(),
              fields.row_height * kRowHeaderScale, kZero));
    fields.legend_frame = fields.calendar_frame.Reduce(
        RectF(fields.cell_width, kZero, kZero,
              fields.cells_frame.Height() + fields.row_height));

    return fields;
  }

  Fields fields_;
};

#endif  // CALENDAR_LAYOUT_HPP
