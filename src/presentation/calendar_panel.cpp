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
#include <array>
#include <cstddef>
#include <functional>
#include <utility>

#include "../domain/calendar_config.hpp"
#include "../domain/date.hpp"
#include "calendar_view_combo_box.hpp"
#include "make_owned.hpp"

CalendarSetupForm::CalendarSetupForm(QWidget* parent) : QWidget(parent) {
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

  auto* band_layout = MakeOwned<QFormLayout>();
  for (std::size_t index = kBandPartCount; index-- > 0;) {
    auto* field = MakeOwned<QDoubleSpinBox>(this);
    field->setDecimals(2);
    field->setRange(0.0, kProportionMax);
    connect(field, &QDoubleSpinBox::valueChanged, this,
            [this](double) { ReportChange(); });
    band_layout->addRow(BandPartLabel(static_cast<BandPart>(index)), field);
    band_proportion_fields_.at(index) = field;
  }

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->addWidget(SectionLabel("View"));
  vertical_layout->addLayout(view_layout);
  vertical_layout->addWidget(SectionLabel("Calendar Span (Years)"));
  vertical_layout->addLayout(span_layout);
  vertical_layout->addWidget(SectionLabel("Visible Elements"));
  vertical_layout->addLayout(elements_layout);
  vertical_layout->addWidget(SectionLabel("Band Proportions"));
  vertical_layout->addLayout(band_layout);
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
            RefreshCoverageProportionState();
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

  const BandProportions& proportions = config.GetBandProportions();
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    band_proportion_fields_.at(index)->setValue(
        static_cast<double>(proportions.at(index)));
  }

  view_->SetView(config.View());
  fit_years_to_entries_->setChecked(config.IsFitYearsToEntries());
  first_year_->setValue(config.FirstYear());
  last_year_->setValue(config.LastYear());
  shows_annual_coverage_->setChecked(config.ShowsAnnualCoverage());

  RefreshSpanLimitsState();
  RefreshCoverageProportionState();
  loading_ = false;
}

CalendarConfig CalendarSetupForm::ReadConfig() const {
  CalendarConfig config;

  BandProportions proportions{};
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    proportions.at(index) =
        static_cast<float>(band_proportion_fields_.at(index)->value());
  }
  config.SetBandProportions(proportions);

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

QString CalendarSetupForm::BandPartLabel(BandPart part) {
  switch (part) {
    case BandPart::kBelowCoverage:
      return "Gap Below Coverage";
    case BandPart::kCoverage:
      return "Coverage";
    case BandPart::kBelowDays:
      return "Gap Below Days";
    case BandPart::kDays:
      return "Days";
    case BandPart::kBelowLabels:
      return "Gap Below Labels";
    case BandPart::kEntryLabels:
      return "Entry Labels";
    case BandPart::kAboveLabels:
      return "Gap Above Labels";
  }
  return {};
}

void CalendarSetupForm::RefreshSpanLimitsState() {
  const bool fit_to_entries = fit_years_to_entries_->isChecked();
  first_year_->setEnabled(!fit_to_entries);
  last_year_->setEnabled(!fit_to_entries);
}

void CalendarSetupForm::RefreshCoverageProportionState() {
  band_proportion_fields_.at(std::to_underlying(BandPart::kCoverage))
      ->setEnabled(shows_annual_coverage_->isChecked());
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
