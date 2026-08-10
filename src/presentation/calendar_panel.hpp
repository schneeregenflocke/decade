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
#include <cstddef>
#include <functional>
#include <sigslot/signal.hpp>
#include <utility>
#include <vector>

#include "../domain/calendar_config.hpp"
#include "../domain/date.hpp"
#include "make_owned.hpp"

// The form that edits a CalendarConfig: the calendar's year span and the
// per-row spacing proportions. It is a pure view — LoadConfig() pushes a config
// into the widgets, ReadConfig() reads the widgets back into a config — so the
// owning panel never reaches into the individual fields.
//
// Row-spacing order: the rendered layout stacks the proportions along the
// rising y-axis (index 0 is the bottom gap, the last index the top gap). The
// form therefore lists them top-to-bottom in reverse index order, so the entry
// at the top of the form is the one drawn at the top of the page.
//
// Qt carries no property grid; the two categories are section headings above a
// QFormLayout each. The spacing rows live in a layout of their own, because
// their number follows the config and they alone get rebuilt.
class CalendarSetupForm : public QWidget {
 public:
  explicit CalendarSetupForm(QWidget* parent);

  void SetOnChanged(std::function<void()> on_changed);

  // Mirrors a config into the form's widgets.
  void LoadConfig(const CalendarConfig& config);

  // Reads the form's widgets back into a fresh config. The number of spacing
  // rows is fixed by the last LoadConfig() — the user only edits values — so
  // this just reads the current widgets back.
  [[nodiscard]] CalendarConfig ReadConfig() const;

 private:
  // Proportions are relative to each other, so the ceiling only has to stay out
  // of the way; two decimals match what the defaults are written in.
  static constexpr double kSpacingMax = 10000.0;
  static constexpr double kDefaultSpacing = 10.0;

  QLabel* SectionLabel(const QString& text);

  // Spacings alternate gap / subrow / gap …, so even indices are gaps and odd
  // indices subrows; the ordinal counts each kind from the bottom up.
  [[nodiscard]] static QString SpacingLabel(std::size_t index);

  // Enables the explicit year limits only while the span is not automatic.
  void RefreshSpanLimitsState();

  // Rebuilds the spacing rows when their count changes (only on LoadConfig).
  // Adding from the highest index down lays them out so the form's top matches
  // the page's top.
  void SyncSpacingRows(std::size_t count);

  void ReportChange();

  QPointer<QCheckBox> auto_span_;
  QPointer<QSpinBox> first_year_;
  QPointer<QSpinBox> last_year_;

  QPointer<QFormLayout> spacing_layout_;
  // Indexed by proportion index (rising y-axis), independent of form order.
  std::vector<QPointer<QDoubleSpinBox>> spacing_fields_;

  std::function<void()> on_changed_;
  bool loading_{false};
};

class CalendarSetupPanel : public QWidget {
 public:
  explicit CalendarSetupPanel(QWidget* parent);

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config);

  sigslot::signal<const CalendarConfig&>& SignalCalendarConfig();

 private:
  void CallbackFormChanged();

  QPointer<CalendarSetupForm> form_;
  CalendarConfig calendar_config_;
  sigslot::signal<const CalendarConfig&> signal_calendar_config_;
};
#endif  // CALENDAR_PANEL_HPP
