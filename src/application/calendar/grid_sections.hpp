#ifndef GRID_SECTIONS_HPP
#define GRID_SECTIONS_HPP

#include "section_context.hpp"

// The calendar grid: the month and year labels around it and the year, month
// and day cells inside it.

namespace calendar_sections {

void BuildCalendarLabels(const SectionContext& ctx);

void BuildYears(const SectionContext& ctx);

void BuildMonths(const SectionContext& ctx);

void BuildDays(const SectionContext& ctx);

}  // namespace calendar_sections

#endif  // GRID_SECTIONS_HPP
