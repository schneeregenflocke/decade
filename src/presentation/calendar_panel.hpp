#ifndef CALENDAR_PANEL_HPP
#define CALENDAR_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include <array>
#include <functional>
#include <optional>

#include "../domain/band_part.hpp"
#include "../domain/calendar_config.hpp"
#include "../domain/calendar_metrics.hpp"
#include "../domain/calendar_sizing.hpp"
#include "calendar_view_combo_box.hpp"
#include "sizing_mode_switch.hpp"

// The form that edits a CalendarConfig: the view, the calendar's year span,
// whether the annual coverage shows, and the calendar's size, one group per
// axis. It is a pure view — LoadConfig() pushes a config into the widgets,
// ReadConfig() reads the widgets back into a config — so the owning panel never
// reaches into the individual fields.
//
// Every size has a field, editable or not: a white field is set by the user, a
// grey one shows what the layout made of the rest — the fitted millimetres, the
// shares a fixed height comes to, the text sizes. Switching an axis either way
// makes the grey values the editable ones, so the drawing does not jump.
//
// The band table lists the parts top to bottom, as the page shows them; the
// layout stacks them along the rising y-axis.
class CalendarSetupForm : public QWidget {
 public:
  explicit CalendarSetupForm(QWidget* parent);

  void SetOnChanged(std::function<void()> on_changed);

  // Mirrors a config into the form's widgets.
  void LoadConfig(const CalendarConfig& config);

  // Reads the form's widgets back into a fresh config.
  [[nodiscard]] CalendarConfig ReadConfig() const;

  void ShowMetrics(const CalendarMetrics& metrics);

 private:
  // One row of the band table. A part that holds text has a text size.
  struct BandPartRow {
    QPointer<QDoubleSpinBox> share;
    QPointer<QDoubleSpinBox> height;
    QPointer<QDoubleSpinBox> text_size;
  };

  struct NumberFormat {
    QString suffix;
    int decimals;
    double step;
    double minimum;
    double maximum;
  };

  static constexpr int kPercentDecimals = 1;

  QLabel* SectionLabel(const QString& text);

  [[nodiscard]] QGroupBox* BuildWidthGroup();

  [[nodiscard]] QGroupBox* BuildHeightGroup();

  // An editable number that reports each committed change.
  QDoubleSpinBox* NumberField(const QString& name, const NumberFormat& format);

  // A number the form derives and the user never edits.
  QDoubleSpinBox* ReadoutField(const QString& name, const NumberFormat& format);

  [[nodiscard]] static QString BandPartLabel(BandPart part);

  [[nodiscard]] static bool HoldsText(BandPart part);

  [[nodiscard]] BandPartRow& Row(BandPart part);

  [[nodiscard]] CalendarSizing ReadSizing() const;

  // The part heights the layout stacks: a hidden coverage takes none.
  [[nodiscard]] BandHeights LaidOutHeights() const;

  void OnWidthSwitched(bool fixed);

  void OnHeightSwitched(bool fixed);

  void TakeOverMetricWidths();

  void TakeOverMetricHeights();

  // Enables the explicit year limits only while the span is not automatic.
  void RefreshSpanLimitsState();

  void RefreshEditability();

  void RefreshReadouts();

  void ReportChange();

  QPointer<CalendarViewComboBox> view_;
  QPointer<QCheckBox> fit_years_to_entries_;
  QPointer<QSpinBox> first_year_;
  QPointer<QSpinBox> last_year_;
  QPointer<QCheckBox> shows_annual_coverage_;

  QPointer<SizingModeSwitch> width_mode_;
  QPointer<QDoubleSpinBox> day_width_;
  QPointer<QDoubleSpinBox> row_labels_width_;
  QPointer<QDoubleSpinBox> row_label_text_size_;
  QPointer<QDoubleSpinBox> legend_entry_width_;

  QPointer<SizingModeSwitch> height_mode_;
  // Indexed by BandPart, independent of the table's order.
  std::array<BandPartRow, kBandPartCount> band_rows_;
  QPointer<QDoubleSpinBox> band_height_;
  QPointer<QDoubleSpinBox> column_labels_height_;
  QPointer<QDoubleSpinBox> column_label_text_size_;
  QPointer<QDoubleSpinBox> legend_height_;

  std::optional<CalendarMetrics> metrics_;
  std::function<void()> on_changed_;
  bool loading_{false};
};

class CalendarSetupPanel : public QWidget {
  Q_OBJECT

 public:
  explicit CalendarSetupPanel(QWidget* parent);

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config);

  void ReceiveCalendarMetrics(const CalendarMetrics& calendar_metrics);

 signals:
  void CalendarConfigEdited(const CalendarConfig& calendar_config);

 private:
  void CallbackFormChanged();

  QPointer<CalendarSetupForm> form_;
  CalendarConfig calendar_config_;
};
#endif  // CALENDAR_PANEL_HPP
