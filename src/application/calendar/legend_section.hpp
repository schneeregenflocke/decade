#ifndef LEGEND_SECTION_HPP
#define LEGEND_SECTION_HPP

#include "../../infrastructure/graphics/rect.hpp"
#include "section_context.hpp"

// The legend below the calendar: one label plus one sample bar per date
// category, and the annual coverage after them while it is shown.

namespace calendar_sections {

// Answers the area the entries take, which runs past the calendar once a fixed
// entry width asks for more than it has.
RectF BuildLegend(const SectionContext& ctx);

}  // namespace calendar_sections

#endif  // LEGEND_SECTION_HPP
