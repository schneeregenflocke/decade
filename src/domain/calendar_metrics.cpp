#include "calendar_metrics.hpp"

#include "calendar_sizing.hpp"

CalendarMetrics::CalendarMetrics(const CalendarSizing::Widths& widths,
                                 const CalendarSizing::Heights& heights,
                                 const TextSizes& text_sizes)
    : widths_(widths), heights_(heights), text_sizes_(text_sizes) {}

const CalendarSizing::Widths& CalendarMetrics::GetWidths() const {
  return widths_;
}

const CalendarSizing::Heights& CalendarMetrics::GetHeights() const {
  return heights_;
}

const CalendarMetrics::TextSizes& CalendarMetrics::GetTextSizes() const {
  return text_sizes_;
}
