#include "area_layout.hpp"

#include <cstddef>
#include <iterator>
#include <numeric>
#include <vector>

#include "rect.hpp"

void ProportionAreaLayout::SetupRowAreas(const RectF& area,
                                         const size_t number_rows) {
  row_areas_.resize(number_rows);

  const auto row_height = area.Height() / static_cast<float>(number_rows);

  for (size_t index = 0; index < row_areas_.size(); ++index) {
    const auto float_index = static_cast<float>(index);
    const auto current_bottom = area.Bottom() + (float_index * row_height);
    row_areas_.at(index) = RectF(area.Left(), area.Right(), current_bottom,
                                 current_bottom + row_height);
  }
}

void ProportionAreaLayout::SetupSubAreas(
    const std::vector<float>& proportions) {
  number_sub_areas_per_row_ =
      proportions.empty() ? 0 : (proportions.size() - 1) / 2;

  sub_areas_.resize(row_areas_.size() * number_sub_areas_per_row_);

  for (size_t index = 0; index < row_areas_.size(); ++index) {
    const auto sections = Section(proportions, row_areas_.at(index).Height());
    std::vector<float> cumulative_sections(sections.size());

    for (size_t subindex = 0; subindex < cumulative_sections.size();
         ++subindex) {
      cumulative_sections.at(subindex) = std::accumulate(
          sections.cbegin(),
          std::next(sections.cbegin(), static_cast<std::ptrdiff_t>(subindex)),
          0.0F);
    }

    for (size_t subindex = 0; subindex < number_sub_areas_per_row_;
         ++subindex) {
      const auto area_index = (index * number_sub_areas_per_row_) + subindex;
      sub_areas_.at(area_index).SetLeft(row_areas_.at(index).Left());
      sub_areas_.at(area_index).SetRight(row_areas_.at(index).Right());
      sub_areas_.at(area_index)
          .SetBottom(row_areas_.at(index).Bottom() +
                     cumulative_sections.at((subindex * 2) + 1));
      sub_areas_.at(area_index)
          .SetTop(row_areas_.at(index).Bottom() +
                  cumulative_sections.at((subindex * 2) + 2));
    }
  }
}

RectF ProportionAreaLayout::GetSubArea(const size_t row,
                                       const size_t sub) const {
  return sub_areas_.at((number_sub_areas_per_row_ * row) + sub);
}

std::vector<float> ProportionAreaLayout::Section(
    const std::vector<float>& proportions, float value) {
  const auto sum =
      std::accumulate(proportions.cbegin(), proportions.cend(), 0.0F);

  std::vector<float> sections;
  sections.reserve(proportions.size());

  for (const auto& proportion : proportions) {
    sections.push_back((proportion / sum) * value);
  }

  return sections;
}
