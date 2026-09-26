#include "calendar_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QFont>
#include <QtWidgets/QAbstractSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
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
#include "../domain/calendar_metrics.hpp"
#include "../domain/calendar_sizing.hpp"
#include "../domain/date.hpp"
#include "../domain/typography.hpp"
#include "calendar_view_combo_box.hpp"
#include "make_owned.hpp"
#include "sizing_mode_switch.hpp"

namespace {

constexpr double kLengthMax = 10000.0;
constexpr double kPercent = 100.0;

double PointsFrom(float millimetres) {
  return static_cast<double>(domain::PointsFromMillimetres(millimetres));
}

// A grey field shows what it holds, not how to change it.
void SetEditable(QDoubleSpinBox& field, bool editable) {
  field.setEnabled(editable);
  field.setButtonSymbols(editable ? QAbstractSpinBox::UpDownArrows
                                  : QAbstractSpinBox::NoButtons);
}

// Sets a value without reporting it as an edit.
void ShowValue(QDoubleSpinBox& field, double value) {
  const QSignalBlocker blocker(&field);
  field.setValue(value);
}

}  // namespace

CalendarSetupForm::CalendarSetupForm(QWidget* parent) : QWidget(parent) {
  view_ = MakeOwned<CalendarViewComboBox>(this);
  fit_years_to_entries_ = MakeOwned<QCheckBox>(this);
  first_year_ = MakeOwned<QSpinBox>(this);
  first_year_->setRange(Date::kMinYear, Date::kMaxYear);
  last_year_ = MakeOwned<QSpinBox>(this);
  last_year_->setRange(Date::kMinYear, Date::kMaxYear);
  shows_annual_coverage_ = MakeOwned<QCheckBox>(this);

  auto* view_layout = MakeOwned<QFormLayout>();
  view_layout->addRow("Calendar view", view_.data());

  auto* span_layout = MakeOwned<QFormLayout>();
  span_layout->addRow("Fit years to entries", fit_years_to_entries_.data());
  span_layout->addRow("First year", first_year_.data());
  span_layout->addRow("Last year", last_year_.data());

  auto* elements_layout = MakeOwned<QFormLayout>();
  elements_layout->addRow("Annual coverage", shows_annual_coverage_.data());

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->addWidget(SectionLabel("View"));
  vertical_layout->addLayout(view_layout);
  vertical_layout->addWidget(SectionLabel("Calendar span (years)"));
  vertical_layout->addLayout(span_layout);
  vertical_layout->addWidget(SectionLabel("Visible elements"));
  vertical_layout->addLayout(elements_layout);
  vertical_layout->addWidget(SectionLabel("Width"));
  vertical_layout->addWidget(BuildWidthGroup());
  vertical_layout->addWidget(SectionLabel("Height"));
  vertical_layout->addWidget(BuildHeightGroup());
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
            RefreshEditability();
            ReportChange();
          });
  connect(first_year_.data(), &QSpinBox::valueChanged, this,
          [this](int) { ReportChange(); });
  connect(last_year_.data(), &QSpinBox::valueChanged, this,
          [this](int) { ReportChange(); });

  RefreshSpanLimitsState();
  RefreshEditability();
}

QGroupBox* CalendarSetupForm::BuildWidthGroup() {
  const NumberFormat day{.suffix = " mm",
                         .decimals = 2,
                         .step = 0.05,
                         .minimum = 0.01,
                         .maximum = kLengthMax};
  const NumberFormat length{.suffix = " mm",
                            .decimals = 1,
                            .step = 1.0,
                            .minimum = 0.0,
                            .maximum = kLengthMax};
  const NumberFormat points{.suffix = " pt",
                            .decimals = 1,
                            .step = 1.0,
                            .minimum = 0.0,
                            .maximum = kLengthMax};

  width_mode_ = MakeOwned<SizingModeSwitch>(this);
  width_mode_->setAccessibleName("Width size");
  day_width_ = NumberField("Day width", day);
  row_labels_width_ = NumberField("Row labels width", length);
  row_label_text_size_ = ReadoutField("Row label text size", points);
  legend_entry_width_ = NumberField("Legend entry width", length);
  width_mode_->SetOnSwitched([this](bool fixed) { OnWidthSwitched(fixed); });

  auto* layout = MakeOwned<QFormLayout>();
  layout->addRow("Size", width_mode_.data());
  layout->addRow("Day", day_width_.data());
  layout->addRow("Row labels", row_labels_width_.data());
  layout->addRow("Row label text", row_label_text_size_.data());
  layout->addRow("Legend entry", legend_entry_width_.data());

  auto* group = MakeOwned<QGroupBox>(this);
  group->setLayout(layout);
  return group;
}

QGroupBox* CalendarSetupForm::BuildHeightGroup() {
  const NumberFormat share{.suffix = "",
                           .decimals = kPercentDecimals,
                           .step = 5.0,
                           .minimum = 0.0,
                           .maximum = kLengthMax};
  const NumberFormat part{.suffix = " mm",
                          .decimals = 2,
                          .step = 0.1,
                          .minimum = 0.0,
                          .maximum = kLengthMax};
  const NumberFormat labels{.suffix = " mm",
                            .decimals = 1,
                            .step = 0.5,
                            .minimum = 0.1,
                            .maximum = kLengthMax};
  const NumberFormat points{.suffix = " pt",
                            .decimals = 1,
                            .step = 1.0,
                            .minimum = 0.0,
                            .maximum = kLengthMax};

  height_mode_ = MakeOwned<SizingModeSwitch>(this);
  height_mode_->setAccessibleName("Height size");
  height_mode_->SetOnSwitched([this](bool fixed) { OnHeightSwitched(fixed); });
  constexpr int kName = 0;
  constexpr int kShare = 1;
  constexpr int kHeight = 2;
  constexpr int kText = 3;
  auto* table = MakeOwned<QGridLayout>();
  auto* mode_label = MakeOwned<QLabel>("Size", this);
  mode_label->setBuddy(height_mode_);
  table->addWidget(mode_label, 0, kName);
  table->addWidget(height_mode_, 0, kShare, 1, kText);
  for (const auto& [column, heading] :
       {std::pair{kShare, "Share"}, std::pair{kHeight, "Height"},
        std::pair{kText, "Text"}}) {
    auto* label = MakeOwned<QLabel>(heading, this);
    label->setAlignment(Qt::AlignHCenter);
    table->addWidget(label, 1, column);
  }

  int grid_row = 2;
  const auto add_row = [&](const QString& name, QDoubleSpinBox* share_field,
                           QDoubleSpinBox* height_field,
                           QDoubleSpinBox* text_field) {
    auto* label = MakeOwned<QLabel>(name, this);
    label->setBuddy(share_field != nullptr ? share_field : height_field);
    table->addWidget(label, grid_row, kName);
    for (const auto& [column, field] :
         {std::pair{kShare, share_field}, std::pair{kHeight, height_field},
          std::pair{kText, text_field}}) {
      if (field != nullptr) {
        table->addWidget(field, grid_row, column);
      }
    }
    ++grid_row;
  };

  for (std::size_t index = kBandPartCount; index-- > 0;) {
    const auto band_part = static_cast<BandPart>(index);
    const QString name = BandPartLabel(band_part);
    BandPartRow& row = band_rows_.at(index);
    row.share = NumberField(name + " share", share);
    row.height = NumberField(name + " height", part);
    if (HoldsText(band_part)) {
      row.text_size = ReadoutField(name + " text size", points);
    }
    add_row(name, row.share, row.height, row.text_size);
  }
  band_height_ = ReadoutField("Band height", part);
  add_row("Band", nullptr, band_height_, nullptr);
  column_labels_height_ = NumberField("Column labels height", labels);
  column_label_text_size_ = ReadoutField("Column label text size", points);
  add_row("Column labels", nullptr, column_labels_height_,
          column_label_text_size_);
  legend_height_ = NumberField("Legend height", labels);
  add_row("Legend", nullptr, legend_height_, nullptr);

  auto* group = MakeOwned<QGroupBox>(this);
  group->setLayout(table);
  return group;
}

QDoubleSpinBox* CalendarSetupForm::NumberField(const QString& name,
                                               const NumberFormat& format) {
  auto* field = MakeOwned<QDoubleSpinBox>(this);
  field->setAccessibleName(name);
  field->setSuffix(format.suffix);
  field->setDecimals(format.decimals);
  field->setSingleStep(format.step);
  field->setRange(format.minimum, format.maximum);
  // A number typed digit by digit passes through values nobody meant: "15"
  // is 1 mm a day before it is 15. The layout follows the committed value.
  field->setKeyboardTracking(false);
  connect(field, &QDoubleSpinBox::valueChanged, this,
          [this](double) { ReportChange(); });
  return field;
}

QDoubleSpinBox* CalendarSetupForm::ReadoutField(const QString& name,
                                                const NumberFormat& format) {
  auto* field = MakeOwned<QDoubleSpinBox>(this);
  field->setAccessibleName(name);
  field->setSuffix(format.suffix);
  field->setDecimals(format.decimals);
  field->setRange(format.minimum, format.maximum);
  SetEditable(*field, false);
  return field;
}

void CalendarSetupForm::SetOnChanged(std::function<void()> on_changed) {
  on_changed_ = std::move(on_changed);
}

void CalendarSetupForm::LoadConfig(const CalendarConfig& config) {
  loading_ = true;

  view_->SetView(config.View());
  fit_years_to_entries_->setChecked(config.IsFitYearsToEntries());
  first_year_->setValue(config.FirstYear());
  last_year_->setValue(config.LastYear());
  shows_annual_coverage_->setChecked(config.ShowsAnnualCoverage());

  const CalendarSizing& sizing = config.Sizing();
  width_mode_->SetFixed(sizing.FixesWidth());
  const CalendarSizing::Widths& widths = sizing.FixedWidths();
  ShowValue(*day_width_, static_cast<double>(widths.day));
  ShowValue(*row_labels_width_, static_cast<double>(widths.row_labels));
  ShowValue(*legend_entry_width_, static_cast<double>(widths.legend_entry));

  height_mode_->SetFixed(sizing.FixesHeight());
  const CalendarSizing::Heights& heights = sizing.FixedHeights();
  const BandProportions& proportions = config.GetBandProportions();
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    ShowValue(*band_rows_.at(index).share,
              static_cast<double>(proportions.at(index)));
    ShowValue(*band_rows_.at(index).height,
              static_cast<double>(heights.parts.at(index)));
  }
  ShowValue(*column_labels_height_, static_cast<double>(heights.column_labels));
  ShowValue(*legend_height_, static_cast<double>(heights.legend));

  RefreshSpanLimitsState();
  RefreshEditability();
  RefreshReadouts();
  loading_ = false;
}

CalendarConfig CalendarSetupForm::ReadConfig() const {
  CalendarConfig config;

  BandProportions proportions{};
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    proportions.at(index) =
        static_cast<float>(band_rows_.at(index).share->value());
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

void CalendarSetupForm::ShowMetrics(const CalendarMetrics& metrics) {
  metrics_ = metrics;
  RefreshReadouts();
}

CalendarSizing CalendarSetupForm::ReadSizing() const {
  CalendarSizing sizing;
  sizing.SetFixesWidth(width_mode_->IsFixed());
  sizing.SetFixedWidths(
      {.day = static_cast<float>(day_width_->value()),
       .row_labels = static_cast<float>(row_labels_width_->value()),
       .legend_entry = static_cast<float>(legend_entry_width_->value())});
  sizing.SetFixesHeight(height_mode_->IsFixed());
  CalendarSizing::Heights heights{
      .parts = {},
      .column_labels = static_cast<float>(column_labels_height_->value()),
      .legend = static_cast<float>(legend_height_->value())};
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    heights.parts.at(index) =
        static_cast<float>(band_rows_.at(index).height->value());
  }
  sizing.SetFixedHeights(heights);
  return sizing;
}

BandHeights CalendarSetupForm::LaidOutHeights() const {
  BandHeights heights{};
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    heights.at(index) =
        static_cast<float>(band_rows_.at(index).height->value());
  }
  if (!shows_annual_coverage_->isChecked()) {
    heights.at(std::to_underlying(BandPart::kCoverage)) = 0.0F;
  }
  return heights;
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
      return "Gap below coverage";
    case BandPart::kCoverage:
      return "Coverage";
    case BandPart::kBelowDays:
      return "Gap below days";
    case BandPart::kDays:
      return "Days";
    case BandPart::kBelowLabels:
      return "Gap below labels";
    case BandPart::kEntryLabels:
      return "Entry labels";
    case BandPart::kAboveLabels:
      return "Gap above labels";
  }
  return {};
}

bool CalendarSetupForm::HoldsText(BandPart part) {
  return part == BandPart::kEntryLabels || part == BandPart::kCoverage;
}

CalendarSetupForm::BandPartRow& CalendarSetupForm::Row(BandPart part) {
  return band_rows_.at(std::to_underlying(part));
}

void CalendarSetupForm::OnWidthSwitched(bool fixed) {
  if (fixed) {
    TakeOverMetricWidths();
  }
  RefreshEditability();
  ReportChange();
}

void CalendarSetupForm::OnHeightSwitched(bool fixed) {
  if (fixed) {
    TakeOverMetricHeights();
  }
  RefreshEditability();
  ReportChange();
}

void CalendarSetupForm::TakeOverMetricWidths() {
  if (!metrics_) {
    return;
  }
  const CalendarSizing::Widths& widths = metrics_->GetWidths();
  ShowValue(*day_width_, static_cast<double>(widths.day));
  ShowValue(*row_labels_width_, static_cast<double>(widths.row_labels));
  ShowValue(*legend_entry_width_, static_cast<double>(widths.legend_entry));
}

void CalendarSetupForm::TakeOverMetricHeights() {
  if (!metrics_) {
    return;
  }
  const CalendarSizing::Heights& heights = metrics_->GetHeights();
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    // A hidden coverage was laid out at nothing; its height stays for when it
    // shows again.
    const bool hidden = index == std::to_underlying(BandPart::kCoverage) &&
                        !shows_annual_coverage_->isChecked();
    if (!hidden) {
      ShowValue(*band_rows_.at(index).height,
                static_cast<double>(heights.parts.at(index)));
    }
  }
  ShowValue(*column_labels_height_, static_cast<double>(heights.column_labels));
  ShowValue(*legend_height_, static_cast<double>(heights.legend));
}

void CalendarSetupForm::RefreshSpanLimitsState() {
  const bool fit_to_entries = fit_years_to_entries_->isChecked();
  first_year_->setEnabled(!fit_to_entries);
  last_year_->setEnabled(!fit_to_entries);
}

void CalendarSetupForm::RefreshEditability() {
  const bool fixes_width = width_mode_->IsFixed();
  for (QDoubleSpinBox* field : {day_width_.data(), row_labels_width_.data(),
                                legend_entry_width_.data()}) {
    SetEditable(*field, fixes_width);
  }

  const bool fixes_height = height_mode_->IsFixed();
  const bool shows_coverage = shows_annual_coverage_->isChecked();
  for (std::size_t index = 0; index < kBandPartCount; ++index) {
    const bool laid_out =
        shows_coverage || index != std::to_underlying(BandPart::kCoverage);
    const BandPartRow& row = band_rows_.at(index);
    SetEditable(*row.share, laid_out && !fixes_height);
    row.share->setSuffix(fixes_height ? " %" : "");
    SetEditable(*row.height, laid_out && fixes_height);
  }
  SetEditable(*column_labels_height_, fixes_height);
  SetEditable(*legend_height_, fixes_height);
}

void CalendarSetupForm::RefreshReadouts() {
  if (metrics_ && !width_mode_->IsFixed()) {
    TakeOverMetricWidths();
  }
  if (metrics_ && !height_mode_->IsFixed()) {
    TakeOverMetricHeights();
  }

  const float band = SumOfParts(LaidOutHeights());
  ShowValue(*band_height_, static_cast<double>(band));
  // A hidden coverage keeps its share against the band it would join, so it
  // comes back at its height.
  if (height_mode_->IsFixed() && band > 0.0F) {
    for (const BandPartRow& row : band_rows_) {
      ShowValue(*row.share,
                kPercent * row.height->value() / static_cast<double>(band));
    }
  }
  for (const BandPart part : {BandPart::kEntryLabels, BandPart::kCoverage}) {
    ShowValue(*Row(part).text_size,
              PointsFrom(static_cast<float>(Row(part).height->value())));
  }

  if (metrics_) {
    const CalendarMetrics::TextSizes& text = metrics_->GetTextSizes();
    ShowValue(*row_label_text_size_, PointsFrom(text.row_labels));
    ShowValue(*column_label_text_size_, PointsFrom(text.column_labels));
  }
}

void CalendarSetupForm::ReportChange() {
  RefreshReadouts();
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

void CalendarSetupPanel::ReceiveCalendarMetrics(
    const CalendarMetrics& calendar_metrics) {
  form_->ShowMetrics(calendar_metrics);
}

void CalendarSetupPanel::CallbackFormChanged() {
  calendar_config_ = form_->ReadConfig();
  emit CalendarConfigEdited(calendar_config_);
}
