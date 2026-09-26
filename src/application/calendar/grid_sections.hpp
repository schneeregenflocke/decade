#ifndef GRID_SECTIONS_HPP
#define GRID_SECTIONS_HPP

#include "../../domain/calendar_metrics.hpp"
#include "section_context.hpp"

// The calendar grid: the column and row labels around it and the year, month
// and day cells inside it.

namespace calendar_sections {

// Answers the em height the labels took.
CalendarMetrics::TextSizes BuildCalendarLabels(const SectionContext& ctx);

void BuildYears(const SectionContext& ctx);

void BuildMonths(const SectionContext& ctx);

void BuildDays(const SectionContext& ctx);

}  // namespace calendar_sections

#endif  // GRID_SECTIONS_HPP
