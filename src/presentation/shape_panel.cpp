#include "shape_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

#include "../domain/date_category.hpp"
#include "../domain/detail/reentry_guard.hpp"
#include "../domain/shape_configuration.hpp"
#include "alpha_slider.hpp"
#include "casts.hpp"
#include "color_button.hpp"
#include "make_owned.hpp"

ShapeSetupPanel::ShapeSetupPanel(QWidget* parent) : QWidget(parent) {
  auto* splitter = MakeOwned<QSplitter>(Qt::Horizontal, this);

  key_list_ = MakeOwned<QListWidget>(splitter);
  auto* detail_widget = MakeOwned<QWidget>(splitter);
  CreateDetailFields(detail_widget);

  splitter->addWidget(key_list_);
  splitter->addWidget(detail_widget);
  splitter->setSizes({kSashPositionPx, kSashPositionPx * 2});

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                      kBorderPx);
  vertical_layout->addWidget(splitter);
  setLayout(vertical_layout);

  connect(key_list_.data(), &QListWidget::currentRowChanged, this,
          [this](int row) { CallbackSelection(row); });
}

void ShapeSetupPanel::ReceiveShapeConfigSet(
    const ShapeConfigSet& shape_config_set) {
  if (editing_) {
    return;
  }
  shape_config_set_ = shape_config_set;
  RebuildKeyList();
  RefreshDetail();
}

void ShapeSetupPanel::ReceiveDateCategories(
    const std::vector<DateCategory>& date_categories) {
  category_names_.clear();
  for (const DateCategory& category : date_categories) {
    category_names_.push_back(category.GetName());
  }
  RebuildKeyList();
}

void ShapeSetupPanel::CreateDetailFields(QWidget* detail_widget) {
  outline_visible_ = MakeOwned<QCheckBox>(detail_widget);
  outline_color_ = MakeOwned<ColorButton>(detail_widget);
  outline_alpha_ = MakeOwned<AlphaSlider>(detail_widget);
  fill_visible_ = MakeOwned<QCheckBox>(detail_widget);
  fill_color_ = MakeOwned<ColorButton>(detail_widget);
  fill_alpha_ = MakeOwned<AlphaSlider>(detail_widget);

  line_width_ = MakeOwned<QDoubleSpinBox>(detail_widget);
  line_width_->setDecimals(2);
  line_width_->setRange(kLineWidthMinMm, kLineWidthMaxMm);
  line_width_->setSingleStep(kLineWidthIncrementMm);

  auto* form_layout = MakeOwned<QFormLayout>();
  form_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx, kBorderPx);
  form_layout->addRow("Outline Visible", outline_visible_.data());
  form_layout->addRow("Outline Color", outline_color_.data());
  form_layout->addRow("Outline Transparency", outline_alpha_.data());
  form_layout->addRow("Fill Visible", fill_visible_.data());
  form_layout->addRow("Fill Color", fill_color_.data());
  form_layout->addRow("Fill Transparency", fill_alpha_.data());
  form_layout->addRow("Line Width (mm)", line_width_.data());
  detail_widget->setLayout(form_layout);

  // One handler for every field: the configuration gets rebuilt from the
  // widgets as a whole, so no field needs a branch of its own.
  const auto on_edit = [this]() { CallbackEdit(); };
  connect(outline_visible_.data(), &QCheckBox::toggled, this,
          [on_edit](bool) { on_edit(); });
  connect(fill_visible_.data(), &QCheckBox::toggled, this,
          [on_edit](bool) { on_edit(); });
  connect(line_width_.data(), &QDoubleSpinBox::valueChanged, this,
          [on_edit](double) { on_edit(); });
  outline_color_->SetOnChanged(on_edit);
  fill_color_->SetOnChanged(on_edit);
  outline_alpha_->SetOnChanged(on_edit);
  fill_alpha_->SetOnChanged(on_edit);
}

std::vector<std::string> ShapeSetupPanel::ConfigurationKeys() const {
  std::vector<std::string> keys;
  keys.reserve(shape_config_set_.FixedConfigurations().size() +
               shape_config_set_.CategoryConfigurations().size());
  for (const auto& config : shape_config_set_.FixedConfigurations()) {
    keys.push_back(config.Key());
  }
  for (const auto& config : shape_config_set_.CategoryConfigurations()) {
    keys.push_back(config.Key());
  }
  return keys;
}

void ShapeSetupPanel::RebuildKeyList() {
  const std::vector<std::string> keys = ConfigurationKeys();

  const QSignalBlocker blocker(key_list_);
  key_list_->clear();
  for (const std::string& key : keys) {
    auto* item = MakeOwned<QListWidgetItem>(LabelFor(key), key_list_);
    item->setData(Qt::UserRole, QString::fromStdString(key));
  }

  if (selected_key_.empty() && !keys.empty()) {
    selected_key_ = keys.front();
  }
  const int row = RowOf(keys, selected_key_);
  if (row < 0) {
    selected_key_ = keys.empty() ? std::string{} : keys.front();
    key_list_->setCurrentRow(keys.empty() ? -1 : 0);
    return;
  }
  key_list_->setCurrentRow(row);
}

QString ShapeSetupPanel::LabelFor(const std::string& key) const {
  const auto& categories = shape_config_set_.CategoryConfigurations();
  for (std::size_t index = 0; index < categories.size(); ++index) {
    if (categories[index].Key() == key && index < category_names_.size()) {
      return QString::fromStdString(category_names_[index]);
    }
  }
  return QString::fromUtf8(ShapeConfigSet::FixedConfigurationLabel(key));
}

int ShapeSetupPanel::RowOf(const std::vector<std::string>& keys,
                           const std::string& key) {
  for (std::size_t index = 0; index < keys.size(); ++index) {
    if (keys[index] == key) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

void ShapeSetupPanel::RefreshDetail() {
  const ShapeConfiguration config =
      shape_config_set_.GetShapeConfiguration(selected_key_);
  const bool has_selection = config.Key() == selected_key_;

  outline_visible_->setEnabled(has_selection);
  outline_color_->setEnabled(has_selection);
  outline_alpha_->setEnabled(has_selection);
  fill_visible_->setEnabled(has_selection);
  fill_color_->setEnabled(has_selection);
  fill_alpha_->setEnabled(has_selection);
  line_width_->setEnabled(has_selection);
  if (!has_selection) {
    return;
  }

  // Loading the widgets fires their change signals; the guard keeps that from
  // reading straight back out as a user edit.
  const domain::detail::ScopedReentryFlag guard(loading_);
  outline_visible_->setChecked(config.OutlineVisible());
  ShowColor(config.OutlineColorDisabled(), outline_color_, outline_alpha_);
  fill_visible_->setChecked(config.FillVisible());
  ShowColor(config.FillColorDisabled(), fill_color_, fill_alpha_);
  line_width_->setValue(static_cast<double>(config.LineWidthDisabled()));
}

void ShapeSetupPanel::ShowColor(const glm::vec4& color,
                                const QPointer<ColorButton>& button,
                                const QPointer<AlphaSlider>& alpha) {
  const QColor qt_color = ToQColor(color);
  button->SetColor(qt_color);
  alpha->SetValue(qt_color.alpha());
}

glm::vec4 ShapeSetupPanel::ReadColor(const QPointer<ColorButton>& button,
                                     const QPointer<AlphaSlider>& alpha) {
  glm::vec4 color = ToGlmVec4(button->Color());
  color[3] = static_cast<float>(alpha->Value()) / kAlphaByteMax;
  return color;
}

void ShapeSetupPanel::CallbackSelection(int row) {
  if (row < 0) {
    return;
  }
  selected_key_ =
      key_list_->item(row)->data(Qt::UserRole).toString().toStdString();
  RefreshDetail();
}

void ShapeSetupPanel::CallbackEdit() {
  if (loading_ || selected_key_.empty()) {
    return;
  }
  const ShapeConfiguration edited{
      selected_key_,
      outline_visible_->isChecked(),
      fill_visible_->isChecked(),
      static_cast<float>(line_width_->value()),
      ShapeConfiguration::OutlineColorValue{
          ReadColor(outline_color_, outline_alpha_)},
      ShapeConfiguration::FillColorValue{ReadColor(fill_color_, fill_alpha_)}};
  if (!shape_config_set_.UpdateConfiguration(edited)) {
    return;
  }
  // The set comes back over the bus while this call still runs; the guard
  // keeps that echo from rebuilding the list under the user's hands.
  const domain::detail::ScopedReentryFlag guard(editing_);
  emit ShapeConfigSetEdited(shape_config_set_);
}
