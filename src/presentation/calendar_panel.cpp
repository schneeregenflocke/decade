#include "calendar_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "../domain/calendar_config.hpp"
#include "../domain/date.hpp"
#include "calendar_view_combo_box.hpp"
#include "make_owned.hpp"

CalendarSetupForm::CalendarSetupForm(QWidget* parent)
    : QWidget(parent), spacing_layout_(MakeOwned<QFormLayout>()) {
  view_ = MakeOwned<CalendarViewComboBox>(this);
  fit_years_to_entries_ = MakeOwned<QCheckBox>(this);
  first_year_ = MakeOwned<QSpinBox>(this);
  first_year_->setRange(Date::kMinYear, Date::kMaxYear);
  last_year_ = MakeOwned<QSpinBox>(this);
  last_year_->setRange(Date::kMinYear, Date::kMaxYear);
  shows_annual_coverage_ = MakeOwned<QCheckBox>(this);

  auto* view_layout = MakeOwned<QFormLayout>();
  view_layout->addRow("Calendar View", view_.data());

  auto* span_layout = MakeOwned<QFormLayout>();
  span_layout->addRow("Fit years to entries", fit_years_to_entries_.data());
  span_layout->addRow("First Year", first_year_.data());
  span_layout->addRow("Last Year", last_year_.data());

  auto* elements_layout = MakeOwned<QFormLayout>();
  elements_layout->addRow("Annual Coverage", shows_annual_coverage_.data());

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->addWidget(SectionLabel("View"));
  vertical_layout->addLayout(view_layout);
  vertical_layout->addWidget(SectionLabel("Calendar Span (Years)"));
  vertical_layout->addLayout(span_layout);
  vertical_layout->addWidget(SectionLabel("Visible Elements"));
  vertical_layout->addLayout(elements_layout);
  vertical_layout->addWidget(SectionLabel("Row Spacing Proportions"));
  vertical_layout->addLayout(spacing_layout_);
  vertical_layout->addStretch(1);
  setLayout(vertical_layout);

  connect(view_.data(), &QComboBox::currentIndexChanged, this,
          [this](int) { ReportChange(); });
  connect(fit_years_to_entries_.data(), &QCheckBox::toggled, this,
          [this](bool) {
            RefreshSpanLimitsState();
            ReportChange();
          });
  connect(shows_annual_coverage_.data(), &QCheckBox::toggled, this,
          [this](bool) {
            RefreshCoverageSpacingState();
            ReportChange();
          });
  connect(first_year_.data(), &QSpinBox::valueChanged, this,
          [this](int) { ReportChange(); });
  connect(last_year_.data(), &QSpinBox::valueChanged, this,
          [this](int) { ReportChange(); });

  RefreshSpanLimitsState();
}

void CalendarSetupForm::SetOnChanged(std::function<void()> on_changed) {
  on_changed_ = std::move(on_changed);
}

void CalendarSetupForm::LoadConfig(const CalendarConfig& config) {
  loading_ = true;

  const auto& proportions = config.GetSpacingProportions();
  SyncSpacingRows(proportions.size());
  for (std::size_t index = 0; index < proportions.size(); ++index) {
    spacing_fields_[index]->setValue(static_cast<double>(proportions[index]));
  }

  view_->SetView(config.View());
  fit_years_to_entries_->setChecked(config.IsFitYearsToEntries());
  first_year_->setValue(config.FirstYear());
  last_year_->setValue(config.LastYear());
  shows_annual_coverage_->setChecked(config.ShowsAnnualCoverage());

  RefreshSpanLimitsState();
  RefreshCoverageSpacingState();
  loading_ = false;
}

CalendarConfig CalendarSetupForm::ReadConfig() const {
  CalendarConfig config;

  std::vector<float> proportions;
  proportions.reserve(spacing_fields_.size());
  for (const auto& field : spacing_fields_) {
    proportions.push_back(static_cast<float>(field->value()));
  }
  config.SetSpacingProportions(proportions);

  config.SetView(view_->View());
  config.SetFitYearsToEntries(fit_years_to_entries_->isChecked());
  config.SetYears(CalendarSpan::YearSpan{.first_year = first_year_->value(),
                                         .last_year = last_year_->value()});
  config.SetShowsAnnualCoverage(shows_annual_coverage_->isChecked());

  return config;
}

QLabel* CalendarSetupForm::SectionLabel(const QString& text) {
  auto* label = MakeOwned<QLabel>(text, this);
  QFont label_font = label->font();
  label_font.setBold(true);
  label->setFont(label_font);
  return label;
}

QString CalendarSetupForm::SpacingLabel(std::size_t index) {
  const std::size_t ordinal = (index / 2) + 1;
  const bool is_subrow = (index % 2) == 1;
  return QString(is_subrow ? "Subrow %1" : "Gap %1").arg(ordinal);
}

void CalendarSetupForm::RefreshSpanLimitsState() {
  const bool fit_to_entries = fit_years_to_entries_->isChecked();
  first_year_->setEnabled(!fit_to_entries);
  last_year_->setEnabled(!fit_to_entries);
}

void CalendarSetupForm::RefreshCoverageSpacingState() {
  if (CalendarConfig::kAnnualCoverageSpacingIndex < spacing_fields_.size()) {
    spacing_fields_[CalendarConfig::kAnnualCoverageSpacingIndex]->setEnabled(
        shows_annual_coverage_->isChecked());
  }
}

void CalendarSetupForm::SyncSpacingRows(std::size_t count) {
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

void CalendarSetupForm::ReportChange() {
  if (!loading_ && on_changed_) {
    on_changed_();
  }
}

CalendarSetupPanel::CalendarSetupPanel(QWidget* parent) : QWidget(parent) {
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

void CalendarSetupPanel::ReceiveCalendarConfig(
    const CalendarConfig& incoming_calendar_config) {
  calendar_config_ = incoming_calendar_config;
  form_->LoadConfig(calendar_config_);
}

void CalendarSetupPanel::CallbackFormChanged() {
  calendar_config_ = form_->ReadConfig();
  emit CalendarConfigEdited(calendar_config_);
}
