#ifndef LEGEND_SECTION_HPP
#define LEGEND_SECTION_HPP

#include "section_context.hpp"

// The legend below the calendar: one label plus one sample bar per date group,
// and the annual sum after them.

namespace calendar_sections {

void BuildLegend(const SectionContext& ctx);

}  // namespace calendar_sections

#endif  // LEGEND_SECTION_HPP
