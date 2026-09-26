#ifndef CALENDAR_LAYOUT_HPP
#define CALENDAR_LAYOUT_HPP

#include <cstddef>
#include <cstdint>
#include <glm/vec3.hpp>
#include <optional>

#include "../../domain/band_part.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../infrastructure/graphics/area_layout.hpp"
#include "../../infrastructure/graphics/rect.hpp"

// The calendar's page geometry, computed once per rebuild from the page size,
// margins, title height and calendar config. It is GL-free and toolkit-free,
// so the whole layout is unit-testable without a GL context.
//
// The areas are deliberately interdependent and therefore computed in one
// pass: print area -> title -> calendar -> cells -> row/sub proportions ->
// label/legend areas. Consumers (the section builders) read the results
// through the accessors; nobody recomputes geometry.
//
// The calendar's height falls into bands: one per year, one for the column
// labels, one for the legend. While the height fits, the rows share the years'
// bands; a fixed height makes each row one band.
//
// Each axis either fits the page or takes the millimetres of the config's
// sizing. A fixed axis anchors the calendar at the top left below the title and
// lets it run past the page's right or bottom edge.
class CalendarLayout {
 public:
  CalendarLayout() = default;

  CalendarLayout(const RectF& page_size, const RectF& page_margin,
                 float title_area_height,
                 const CalendarConfig& calendar_config);

  [[nodiscard]] const glm::vec3& PrintAreaOrigin() const;
  [[nodiscard]] const RectF& PrintArea() const;
  [[nodiscard]] const RectF& TitleArea() const;
  [[nodiscard]] const RectF& CalendarArea() const;
  [[nodiscard]] const RectF& CellsArea() const;
  [[nodiscard]] const RectF& XLabelsArea() const;
  [[nodiscard]] const RectF& YLabelsArea() const;
  [[nodiscard]] const RectF& LegendArea() const;
  [[nodiscard]] float DayWidth() const;

  // The width of one legend entry, its label and its sample together: the
  // legend area shared out while the width fits, the fixed width otherwise.
  [[nodiscard]] float LegendEntryWidth(std::size_t entry_count) const;

  [[nodiscard]] RectF GetRowArea(std::size_t row) const;

  // Where a row draws the given part. Precondition: `part` is one of the three
  // rows of content, not a gap.
  [[nodiscard]] RectF GetPartArea(std::size_t row, BandPart part) const;

  // The height a part takes within one year's band. Texts inside a row and the
  // legend's samples take it, so they keep their size in every view.
  [[nodiscard]] float BandPartHeight(BandPart part) const;

  [[nodiscard]] const BandHeights& BandPartHeights() const;

 private:
  static constexpr float kZero = 0.0F;
  static constexpr float kDefaultMargin = 5.0F;
  static constexpr float kCalendarColumns = 13.0F;
  static constexpr std::size_t kLabelAndLegendBands = 2;

  struct Heights {
    float band;
    float column_labels;
    float legend;
  };

  struct Widths {
    float row_labels;
    float cells;
  };

  [[nodiscard]] static Heights ComputeHeights(const RectF& available,
                                              const CalendarConfig& config);

  [[nodiscard]] static Widths ComputeWidths(const RectF& available,
                                            const CalendarConfig& config,
                                            std::int64_t row_days);

  // The index of a content part among the areas the proportions yield, which
  // skip the gaps.
  [[nodiscard]] static std::size_t ContentIndex(BandPart part);

  // All computed geometry in one aggregate, so the constructor can initialise
  // it from a single pure function (rather than assigning members in its body).
  struct Fields {
    ProportionAreaLayout proportions;
    BandHeights band_part_heights{};
    glm::vec3 print_area_origin{0.0F};
    RectF print_area;
    RectF title_area;
    RectF calendar_area;
    RectF cells_area;
    RectF x_labels_area;
    RectF y_labels_area;
    RectF legend_area;
    float day_width{0.0F};
    std::optional<float> fixed_legend_entry_width;
  };

  static Fields Compute(const RectF& page_size, const RectF& page_margin,
                        float title_area_height,
                        const CalendarConfig& calendar_config);

  Fields fields_;
};

#endif  // CALENDAR_LAYOUT_HPP
