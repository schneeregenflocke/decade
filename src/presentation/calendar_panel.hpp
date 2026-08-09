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
  explicit CalendarSetupForm(QWidget* parent)
      : QWidget(parent), spacing_layout_(MakeOwned<QFormLayout>()) {
    auto_span_ = MakeOwned<QCheckBox>(this);
    first_year_ = MakeOwned<QSpinBox>(this);
    first_year_->setRange(Date::kMinYear, Date::kMaxYear);
    last_year_ = MakeOwned<QSpinBox>(this);
    last_year_->setRange(Date::kMinYear, Date::kMaxYear);

    auto* span_layout = MakeOwned<QFormLayout>();
    span_layout->addRow("Auto Span", auto_span_.data());
    span_layout->addRow("First Year", first_year_.data());
    span_layout->addRow("Last Year", last_year_.data());

    auto* vertical_layout = MakeOwned<QVBoxLayout>();
    vertical_layout->addWidget(SectionLabel("Calendar Span (Years)"));
    vertical_layout->addLayout(span_layout);
    vertical_layout->addWidget(SectionLabel("Row Spacing Proportions"));
    vertical_layout->addLayout(spacing_layout_);
    vertical_layout->addStretch(1);
    setLayout(vertical_layout);

    connect(auto_span_.data(), &QCheckBox::toggled, this, [this](bool) {
      RefreshSpanLimitsState();
      ReportChange();
    });
    connect(first_year_.data(), &QSpinBox::valueChanged, this,
            [this](int) { ReportChange(); });
    connect(last_year_.data(), &QSpinBox::valueChanged, this,
            [this](int) { ReportChange(); });

    RefreshSpanLimitsState();
  }

  void SetOnChanged(std::function<void()> on_changed) {
    on_changed_ = std::move(on_changed);
  }

  // Mirrors a config into the form's widgets.
  void LoadConfig(const CalendarConfig& config) {
    loading_ = true;

    const auto& proportions = config.GetSpacingProportions();
    SyncSpacingRows(proportions.size());
    for (std::size_t index = 0; index < proportions.size(); ++index) {
      spacing_fields_[index]->setValue(static_cast<double>(proportions[index]));
    }

    auto_span_->setChecked(config.IsAutoCalendarSpan());
    first_year_->setValue(config.GetSpanLimitsYears()[0]);
    last_year_->setValue(config.GetSpanLimitsYears()[1]);

    RefreshSpanLimitsState();
    loading_ = false;
  }

  // Reads the form's widgets back into a fresh config. The number of spacing
  // rows is fixed by the last LoadConfig() — the user only edits values — so
  // this just reads the current widgets back.
  [[nodiscard]] CalendarConfig ReadConfig() const {
    CalendarConfig config;

    std::vector<float> proportions;
    proportions.reserve(spacing_fields_.size());
    for (const auto& field : spacing_fields_) {
      proportions.push_back(static_cast<float>(field->value()));
    }
    config.SetSpacingProportions(proportions);

    config.SetAutoCalendarSpan(auto_span_->isChecked());
    config.SetSpan(CalendarSpan::YearSpan{.first_year = first_year_->value(),
                                          .last_year = last_year_->value()});

    return config;
  }

 private:
  // Proportions are relative to each other, so the ceiling only has to stay out
  // of the way; two decimals match what the defaults are written in.
  static constexpr double kSpacingMax = 10000.0;
  static constexpr double kDefaultSpacing = 10.0;

  QLabel* SectionLabel(const QString& text) {
    auto* label = MakeOwned<QLabel>(text, this);
    QFont label_font = label->font();
    label_font.setBold(true);
    label->setFont(label_font);
    return label;
  }

  // Spacings alternate gap / subrow / gap …, so even indices are gaps and odd
  // indices subrows; the ordinal counts each kind from the bottom up.
  [[nodiscard]] static QString SpacingLabel(std::size_t index) {
    const std::size_t ordinal = (index / 2) + 1;
    const bool is_subrow = (index % 2) == 1;
    return QString(is_subrow ? "Subrow %1" : "Gap %1").arg(ordinal);
  }

  // Enables the explicit year limits only while the span is not automatic.
  void RefreshSpanLimitsState() {
    const bool auto_span = auto_span_->isChecked();
    first_year_->setEnabled(!auto_span);
    last_year_->setEnabled(!auto_span);
  }

  // Rebuilds the spacing rows when their count changes (only on LoadConfig).
  // Adding from the highest index down lays them out so the form's top matches
  // the page's top.
  void SyncSpacingRows(std::size_t count) {
    if (count == spacing_fields_.size()) {
      return;
    }

    // The layout owns the widgets it holds; clearing it deletes them, which is
    // why the index vector gets rebuilt rather than patched.
    while (spacing_layout_->rowCount() > 0) {
      spacing_layout_->removeRow(0);
    }
    spacing_fields_.assign(count, nullptr);

    for (std::size_t index = count; index-- > 0;) {
      auto* field = MakeOwned<QDoubleSpinBox>(this);
      field->setDecimals(2);
      field->setRange(0.0, kSpacingMax);
      field->setValue(kDefaultSpacing);
      connect(field, &QDoubleSpinBox::valueChanged, this,
              [this](double) { ReportChange(); });
      spacing_layout_->addRow(SpacingLabel(index), field);
      spacing_fields_[index] = field;
    }
  }

  void ReportChange() {
    if (!loading_ && on_changed_) {
      on_changed_();
    }
  }

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
  explicit CalendarSetupPanel(QWidget* parent) : QWidget(parent) {
    constexpr int kBorderPx = 5;

    auto* scroll_area = MakeOwned<QScrollArea>(this);
    auto* form = MakeOwned<CalendarSetupForm>(scroll_area);
    form_ = form;
    scroll_area->setWidget(form);
    scroll_area->setWidgetResizable(true);

    auto* vertical_layout = MakeOwned<QVBoxLayout>();
    vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                        kBorderPx);
    vertical_layout->addWidget(scroll_area);
    setLayout(vertical_layout);

    form_->SetOnChanged([this]() { CallbackFormChanged(); });
    form_->LoadConfig(calendar_config_);
  }

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config) {
    calendar_config_ = incoming_calendar_config;
    form_->LoadConfig(calendar_config_);
  }

  sigslot::signal<const CalendarConfig&>& SignalCalendarConfig() {
    return signal_calendar_config_;
  }

 private:
  void CallbackFormChanged() {
    calendar_config_ = form_->ReadConfig();
    signal_calendar_config_(calendar_config_);
  }

  QPointer<CalendarSetupForm> form_;
  CalendarConfig calendar_config_;
  sigslot::signal<const CalendarConfig&> signal_calendar_config_;
};
#endif  // CALENDAR_PANEL_HPP
