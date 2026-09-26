#ifndef CALENDAR_SIZING_HPP
#define CALENDAR_SIZING_HPP

// Pure domain value: how the calendar takes its size, one axis at a time —
// fitted to the page, or fixed in millimetres, running past the page where it
// has to. An axis keeps its millimetres while it fits, so switching back
// restores them.
class CalendarSizing {
 public:
  struct Widths {
    float day;
    float row_labels;
    float legend_entry;
  };

  struct Heights {
    float band;
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
  static constexpr float kDefaultBandHeight = 6.0F;

  bool fixes_width_{false};
  Widths widths_{.day = kDefaultDayWidth,
                 .row_labels = kDefaultRowLabelsWidth,
                 .legend_entry = kDefaultLegendEntryWidth};
  bool fixes_height_{false};
  Heights heights_{.band = kDefaultBandHeight,
                   .column_labels = kDefaultBandHeight,
                   .legend = kDefaultBandHeight};
};

#endif  // CALENDAR_SIZING_HPP
