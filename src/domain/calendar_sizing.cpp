#include "calendar_sizing.hpp"

bool CalendarSizing::FixesWidth() const { return fixes_width_; }

void CalendarSizing::SetFixesWidth(bool fixes) { fixes_width_ = fixes; }

const CalendarSizing::Widths& CalendarSizing::FixedWidths() const {
  return widths_;
}

void CalendarSizing::SetFixedWidths(const Widths& widths) { widths_ = widths; }

bool CalendarSizing::FixesHeight() const { return fixes_height_; }

void CalendarSizing::SetFixesHeight(bool fixes) { fixes_height_ = fixes; }

const CalendarSizing::Heights& CalendarSizing::FixedHeights() const {
  return heights_;
}

void CalendarSizing::SetFixedHeights(const Heights& heights) {
  heights_ = heights;
}
