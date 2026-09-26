#ifndef CALENDAR_SIZING_HPP
#define CALENDAR_SIZING_HPP

#include "band_part.hpp"

// Pure domain value: how the calendar takes its size, one axis at a time —
// fitted to the page, or fixed in millimetres, running past the page where it
// has to. A fixed height sets every part of a band, and the band is their sum.
class CalendarSizing {
 public:
  struct Widths {
    float day;
    float row_labels;
    float legend_entry;
  };

  struct Heights {
    BandHeights parts;
    float column_labels;
    float legend;
  };

  [[nodiscard]] bool FixesWidth() const;
  void SetFixesWidth(bool fixes);

  [[nodiscard]] const Widths& FixedWidths() const;
  void SetFixedWidths(const Widths& widths);

  [[nodiscard]] bool FixesHeight() const;
  void SetFixesHeight(bool fixes);

  [[nodiscard]] const Heights& FixedHeights() const;
  void SetFixedHeights(const Heights& heights);

 private:
  static constexpr float kDefaultDayWidth = 0.7F;
  static constexpr float kDefaultRowLabelsWidth = 20.0F;
  static constexpr float kDefaultLegendEntryWidth = 50.0F;
  static constexpr float kDefaultGapHeight = 0.3F;
  static constexpr float kDefaultSpacingHeight = 0.6F;
  static constexpr float kDefaultRowHeight = 1.4F;
  static constexpr float kDefaultLabelsHeight = 6.0F;

  bool fixes_width_{false};
  Widths widths_{.day = kDefaultDayWidth,
                 .row_labels = kDefaultRowLabelsWidth,
                 .legend_entry = kDefaultLegendEntryWidth};
  bool fixes_height_{false};
  Heights heights_{
      .parts = {kDefaultGapHeight, kDefaultRowHeight, kDefaultSpacingHeight,
                kDefaultRowHeight, kDefaultSpacingHeight, kDefaultRowHeight,
                kDefaultGapHeight},
      .column_labels = kDefaultLabelsHeight,
      .legend = kDefaultLabelsHeight};
};

#endif  // CALENDAR_SIZING_HPP
