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

#include "../domain/band_part.hpp"
#include "../domain/calendar_config.hpp"
#include "../domain/calendar_sizing.hpp"
#include "../domain/date.hpp"
#include "../domain/font_config.hpp"
#include "../domain/typography.hpp"
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

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->addWidget(SectionLabel("View"));
  vertical_layout->addLayout(view_layout);
  vertical_layout->addWidget(SectionLabel("Calendar Span (Years)"));
  vertical_layout->addLayout(span_layout);
  vertical_layout->addWidget(SectionLabel("Visible Elements"));
  vertical_layout->addLayout(elements_layout);
  vertical_layout->addWidget(SectionLabel("Calendar Size"));
  vertical_layout->addLayout(BuildSizeLayout());
  vertical_layout->addWidget(SectionLabel("Band Proportions"));
  vertical_layout->addLayout(BuildBandLayout());
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
  RefreshSizingState();
}

QFormLayout* CalendarSetupForm::BuildSizeLayout() {
  fixes_width_ = MakeOwned<QCheckBox>(this);
  day_width_ = MillimetreField();
  row_labels_width_ = MillimetreField();
  row_label_size_ = MakeOwned<QLabel>(this);
  legend_entry_width_ = MillimetreField();
  fixes_height_ = MakeOwned<QCheckBox>(this);
  band_height_ = MillimetreField();
  day_height_ = MakeOwned<QLabel>(this);
  entry_label_size_ = MakeOwned<QLabel>(this);
  coverage_label_size_ = MakeOwned<QLabel>(this);
  column_labels_height_ = MillimetreField();
  legend_height_ = MillimetreField();

  for (const QPointer<QCheckBox>& axis : {fixes_width_, fixes_height_}) {
    connect(axis.data(), &QCheckBox::toggled, this, [this](bool) {
      RefreshSizingState();
      ReportChange();
    });
  }

  auto* layout = MakeOwned<QFormLayout>();
  layout->addRow("Fixed Width", fixes_width_.data());
  layout->addRow("Day Width (mm)", day_width_.data());
  layout->addRow("Row Label Width (mm)", row_labels_width_.data());
  layout->addRow("Row Label Size (pt)", row_label_size_.data());
  layout->addRow("Legend Entry Width (mm)", legend_entry_width_.data());
  layout->addRow("Fixed Height", fixes_height_.data());
  layout->addRow("Band Height (mm)", band_height_.data());
  layout->addRow("Day Height (mm)", day_height_.data());
  layout->addRow("Entry Label Size (pt)", entry_label_size_.data());
  layout->addRow("Coverage Label Size (pt)", coverage_label_size_.data());
  layout->addRow("Column Label Height (mm)", column_labels_height_.data());
  layout->addRow("Legend Height (mm)", legend_height_.data());
  return layout;
}

QFormLayout* CalendarSetupForm::BuildBandLayout() {
  auto* layout = MakeOwned<QFormLayout>();
  for (std::size_t index = kBandPartCount; index-- > 0;) {
    auto* field = MakeOwned<QDoubleSpinBox>(this);
    field->setDecimals(2);
    field->setRange(0.0, kProportionMax);
    connect(field, &QDoubleSpinBox::valueChanged, this,
            [this](double) { ReportChange(); });
    layout->addRow(BandPartLabel(static_cast<BandPart>(index)), field);
    band_proportion_fields_.at(index) = field;
  }
  return layout;
}

QDoubleSpinBox* CalendarSetupForm::MillimetreField() {
  auto* field = MakeOwned<QDoubleSpinBox>(this);
  field->setDecimals(2);
  field->setRange(kMillimetreMin, kMillimetreMax);
  connect(field, &QDoubleSpinBox::valueChanged, this,
          [this](double) { ReportChange(); });
  return field;
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

  const CalendarSizing& sizing = config.Sizing();
  fixes_width_->setChecked(sizing.FixesWidth());
  day_width_->setValue(static_cast<double>(sizing.FixedWidths().day));
  row_labels_width_->setValue(
      static_cast<double>(sizing.FixedWidths().row_labels));
  legend_entry_width_->setValue(
      static_cast<double>(sizing.FixedWidths().legend_entry));
  fixes_height_->setChecked(sizing.FixesHeight());
  band_height_->setValue(static_cast<double>(sizing.FixedHeights().band));
  column_labels_height_->setValue(
      static_cast<double>(sizing.FixedHeights().column_labels));
  legend_height_->setValue(static_cast<double>(sizing.FixedHeights().legend));

  RefreshSpanLimitsState();
  RefreshCoverageProportionState();
  RefreshSizingState();
  RefreshDerivedSizes();
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
  config.SetSizing(ReadSizing());

  return config;
}

CalendarSizing CalendarSetupForm::ReadSizing() const {
  CalendarSizing sizing;
  sizing.SetFixesWidth(fixes_width_->isChecked());
  sizing.SetFixedWidths(
      {.day = static_cast<float>(day_width_->value()),
       .row_labels = static_cast<float>(row_labels_width_->value()),
       .legend_entry = static_cast<float>(legend_entry_width_->value())});
  sizing.SetFixesHeight(fixes_height_->isChecked());
  sizing.SetFixedHeights(
      {.band = static_cast<float>(band_height_->value()),
       .column_labels = static_cast<float>(column_labels_height_->value()),
       .legend = static_cast<float>(legend_height_->value())});
  return sizing;
}

void CalendarSetupForm::ShowRowLabelSize(float points) {
  row_label_size_->setText(
      QString::number(static_cast<double>(points), 'f', 1));
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

void CalendarSetupForm::RefreshSizingState() {
  const bool fixes_width = fixes_width_->isChecked();
  for (QDoubleSpinBox* field : {day_width_.data(), row_labels_width_.data(),
                                legend_entry_width_.data()}) {
    field->setEnabled(fixes_width);
  }
  const bool fixes_height = fixes_height_->isChecked();
  for (QDoubleSpinBox* field :
       {band_height_.data(), column_labels_height_.data(),
        legend_height_.data()}) {
    field->setEnabled(fixes_height);
  }
}

void CalendarSetupForm::RefreshDerivedSizes() {
  const CalendarConfig config = ReadConfig();
  if (!config.Sizing().FixesHeight()) {
    for (QLabel* label : {day_height_.data(), entry_label_size_.data(),
                          coverage_label_size_.data()}) {
      label->setText("fits the page");
    }
    return;
  }
  const float band = config.Sizing().FixedHeights().band;
  const auto part_height = [&](BandPart part) {
    return band * config.LaidOutShare(part);
  };
  day_height_->setText(QString::number(
      static_cast<double>(part_height(BandPart::kDays)), 'f', 2));
  const auto points = [&](BandPart part) {
    return QString::number(
        static_cast<double>(domain::PointsFromMillimetres(part_height(part))),
        'f', 1);
  };
  entry_label_size_->setText(points(BandPart::kEntryLabels));
  coverage_label_size_->setText(points(BandPart::kCoverage));
}

void CalendarSetupForm::ReportChange() {
  RefreshDerivedSizes();
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

void CalendarSetupPanel::ReceiveFontConfig(const FontConfig& font_config) {
  form_->ShowRowLabelSize(font_config.SizePoints());
}

void CalendarSetupPanel::CallbackFormChanged() {
  calendar_config_ = form_->ReadConfig();
  emit CalendarConfigEdited(calendar_config_);
}
