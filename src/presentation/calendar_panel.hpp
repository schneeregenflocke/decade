#ifndef CALENDAR_PANEL_HPP
#define CALENDAR_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <array>
#include <functional>
#include <utility>

#include "../domain/calendar_config.hpp"
#include "../domain/date.hpp"
#include "calendar_view_combo_box.hpp"
#include "make_owned.hpp"

// The form that edits a CalendarConfig: the view, the calendar's year span,
// whether the annual coverage shows, and the band proportions. It is a pure
// view — LoadConfig() pushes a config into the widgets, ReadConfig() reads the
// widgets back into a config — so the owning panel never reaches into the
// individual fields.
//
// The band stacks its parts along the rising y-axis, so the form lists them in
// reverse: the part at the top of the form is the one drawn at the top of the
// band.
//
// Qt carries no property grid; the categories are section headings above a
// QFormLayout each.
class CalendarSetupForm : public QWidget {
 public:
  explicit CalendarSetupForm(QWidget* parent);

  void SetOnChanged(std::function<void()> on_changed);

  // Mirrors a config into the form's widgets.
  void LoadConfig(const CalendarConfig& config);

  // Reads the form's widgets back into a fresh config.
  [[nodiscard]] CalendarConfig ReadConfig() const;

 private:
  // Proportions are relative to each other, so the ceiling only has to stay out
  // of the way; two decimals match what the defaults are written in.
  static constexpr double kProportionMax = 10000.0;

  QLabel* SectionLabel(const QString& text);

  [[nodiscard]] static QString BandPartLabel(BandPart part);

  // Enables the explicit year limits only while the span is not automatic.
  void RefreshSpanLimitsState();

  // A hidden annual coverage lays out no part, so its proportion has no effect.
  void RefreshCoverageProportionState();

  void ReportChange();

  QPointer<CalendarViewComboBox> view_;
  QPointer<QCheckBox> fit_years_to_entries_;
  QPointer<QSpinBox> first_year_;
  QPointer<QSpinBox> last_year_;
  QPointer<QCheckBox> shows_annual_coverage_;

  // Indexed by BandPart, independent of form order.
  std::array<QPointer<QDoubleSpinBox>, kBandPartCount> band_proportion_fields_;

  std::function<void()> on_changed_;
  bool loading_{false};
};

class CalendarSetupPanel : public QWidget {
  Q_OBJECT

 public:
  explicit CalendarSetupPanel(QWidget* parent);

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config);

 signals:
  void CalendarConfigEdited(const CalendarConfig& calendar_config);

 private:
  void CallbackFormChanged();

  QPointer<CalendarSetupForm> form_;
  CalendarConfig calendar_config_;
};
#endif  // CALENDAR_PANEL_HPP
