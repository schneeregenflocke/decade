#ifndef CALENDAR_METRICS_HPP
#define CALENDAR_METRICS_HPP

#include "calendar_sizing.hpp"

// Pure domain value: the sizes the calendar came out at on its last layout, in
// millimetres, fitted and fixed axes alike. The Timeframe form shows them, and
// fixing an axis starts from them, so the drawing does not jump.
class CalendarMetrics {
 public:
  // The em height of the label texts, which a fixed axis fits to their frames.
  struct TextSizes {
    float row_labels;
    float column_labels;
  };

  CalendarMetrics() = default;

  CalendarMetrics(const CalendarSizing::Widths& widths,
                  const CalendarSizing::Heights& heights,
                  const TextSizes& text_sizes);

  [[nodiscard]] const CalendarSizing::Widths& GetWidths() const;

  [[nodiscard]] const CalendarSizing::Heights& GetHeights() const;

  [[nodiscard]] const TextSizes& GetTextSizes() const;

 private:
  CalendarSizing::Widths widths_{};
  CalendarSizing::Heights heights_{};
  TextSizes text_sizes_{};
};

#endif  // CALENDAR_METRICS_HPP
