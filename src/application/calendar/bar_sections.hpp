#ifndef BAR_SECTIONS_HPP
#define BAR_SECTIONS_HPP

#include "section_context.hpp"

// The date bars with their labels and pick boxes, plus the per-year totals
// beside them — both read the same bar data, hence one file.

namespace calendar_sections {

[[nodiscard]] BarSceneResult BuildBars(const SectionContext& ctx);

void BuildYearTotals(const SectionContext& ctx);

}  // namespace calendar_sections

#endif  // BAR_SECTIONS_HPP
