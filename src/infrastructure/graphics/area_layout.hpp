#ifndef AREA_LAYOUT_HPP
#define AREA_LAYOUT_HPP

#include <cstddef>
#include <vector>

#include "rect.hpp"

class ProportionAreaLayout {
 public:
  void SetupRowAreas(const RectF& area, size_t number_rows);

  // The proportions alternate gap / subrow / … / gap, so a valid set has an
  // odd length 2k+1 yielding k subrows. The guard keeps a degenerate (empty)
  // set from underflowing the unsigned subtraction.
  void SetupSubAreas(const std::vector<float>& proportions);

  [[nodiscard]] RectF GetSubArea(size_t row, size_t sub) const;

 private:
  [[nodiscard]] static std::vector<float> Section(
      const std::vector<float>& proportions, float value);

  std::vector<RectF> row_areas_;
  std::vector<RectF> sub_areas_;
  size_t number_sub_areas_per_row_{0};
};
#endif  // AREA_LAYOUT_HPP
