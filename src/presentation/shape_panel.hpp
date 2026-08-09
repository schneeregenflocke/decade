#ifndef SHAPE_PANEL_HPP
#define SHAPE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <glm/vec4.hpp>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../domain/detail/reentry_guard.hpp"
#include "../domain/shape_configuration.hpp"
#include "alpha_slider.hpp"
#include "casts.hpp"
#include "color_button.hpp"
#include "make_owned.hpp"

// The home of the shape configurations: the list on the left names them, the
// fields on the right edit the selected one. The scene tree mirrors the same
// values and stays read-only — this is where they get changed (#65).
//
// Master and detail rather than one long form, because the set grows with the
// date groups: the fixed configurations plus one entry per group.
class ShapeSetupPanel : public QWidget {
 public:
  explicit ShapeSetupPanel(QWidget* parent) : QWidget(parent) {
    auto* splitter = MakeOwned<QSplitter>(Qt::Horizontal, this);

    name_list_ = MakeOwned<QListWidget>(splitter);
    auto* detail_widget = MakeOwned<QWidget>(splitter);
    CreateDetailFields(detail_widget);

    splitter->addWidget(name_list_);
    splitter->addWidget(detail_widget);
    splitter->setSizes({kSashPositionPx, kSashPositionPx * 2});

    auto* vertical_layout = MakeOwned<QVBoxLayout>();
    vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                        kBorderPx);
    vertical_layout->addWidget(splitter);
    setLayout(vertical_layout);

    connect(name_list_.data(), &QListWidget::currentRowChanged, this,
            [this](int row) { CallbackSelection(row); });
  }

  // The set arrives whole and replaces what stands here. The selection follows
  // the name, not the row: a new date group adds an entry and would otherwise
  // shift the selection to a different configuration.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set) {
    if (editing_) {
      return;
    }
    shape_config_set_ = shape_config_set;
    RebuildNameList();
    RefreshDetail();
  }

  sigslot::signal<const ShapeConfigSet&>& SignalShapeConfigSet() {
    return signal_shape_config_set_;
  }

 private:
  static constexpr float kAlphaByteMax = 255.0F;
  static constexpr int kSashPositionPx = 180;
  static constexpr int kBorderPx = 5;
  static constexpr double kLineWidthMinMm = 0.0;
  static constexpr double kLineWidthMaxMm = 20.0;
  static constexpr double kLineWidthIncrementMm = 0.1;

  void CreateDetailFields(QWidget* detail_widget) {
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

  // Every configuration of the set in one list: the fixed ones first, the
  // per-date-group ones after them, in the order the set holds them.
  [[nodiscard]] std::vector<std::string> ConfigurationNames() const {
    std::vector<std::string> names;
    names.reserve(shape_config_set_.FixedConfigurations().size() +
                  shape_config_set_.GroupConfigurations().size());
    for (const auto& config : shape_config_set_.FixedConfigurations()) {
      names.push_back(config.Name());
    }
    for (const auto& config : shape_config_set_.GroupConfigurations()) {
      names.push_back(config.Name());
    }
    return names;
  }

  void RebuildNameList() {
    const std::vector<std::string> names = ConfigurationNames();

    const QSignalBlocker blocker(name_list_);
    name_list_->clear();
    for (const std::string& name : names) {
      name_list_->addItem(QString::fromStdString(name));
    }

    if (selected_name_.empty() && !names.empty()) {
      selected_name_ = names.front();
    }
    const int row = RowOf(names, selected_name_);
    if (row < 0) {
      selected_name_ = names.empty() ? std::string{} : names.front();
      name_list_->setCurrentRow(names.empty() ? -1 : 0);
      return;
    }
    name_list_->setCurrentRow(row);
  }

  static int RowOf(const std::vector<std::string>& names,
                   const std::string& name) {
    for (std::size_t index = 0; index < names.size(); ++index) {
      if (names[index] == name) {
        return static_cast<int>(index);
      }
    }
    return -1;
  }

  // The fields show the configured values, not the ones visibility cleans up:
  // switching a fill off and on again must give back the colour that was set.
  void RefreshDetail() {
    const ShapeConfiguration config =
        shape_config_set_.GetShapeConfiguration(selected_name_);
    const bool has_selection = config.Name() == selected_name_;

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

  static void ShowColor(const glm::vec4& color,
                        const QPointer<ColorButton>& button,
                        const QPointer<AlphaSlider>& alpha) {
    const QColor qt_color = ToQColor(color);
    button->SetColor(qt_color);
    alpha->SetValue(qt_color.alpha());
  }

  static glm::vec4 ReadColor(const QPointer<ColorButton>& button,
                             const QPointer<AlphaSlider>& alpha) {
    glm::vec4 color = ToGlmVec4(button->Color());
    color[3] = static_cast<float>(alpha->Value()) / kAlphaByteMax;
    return color;
  }

  void CallbackSelection(int row) {
    if (row < 0) {
      return;
    }
    selected_name_ = name_list_->item(row)->text().toStdString();
    RefreshDetail();
  }

  void CallbackEdit() {
    if (loading_ || selected_name_.empty()) {
      return;
    }
    const ShapeConfiguration edited{
        selected_name_,
        outline_visible_->isChecked(),
        fill_visible_->isChecked(),
        static_cast<float>(line_width_->value()),
        ShapeConfiguration::OutlineColorValue{
            ReadColor(outline_color_, outline_alpha_)},
        ShapeConfiguration::FillColorValue{
            ReadColor(fill_color_, fill_alpha_)}};
    if (!shape_config_set_.UpdateConfiguration(edited)) {
      return;
    }
    // The set comes back over the bus while this call still runs; the guard
    // keeps that echo from rebuilding the list under the user's hands.
    const domain::detail::ScopedReentryFlag guard(editing_);
    signal_shape_config_set_(shape_config_set_);
  }

  ShapeConfigSet shape_config_set_;
  std::string selected_name_;
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;

  QPointer<QListWidget> name_list_;
  QPointer<QCheckBox> outline_visible_;
  QPointer<ColorButton> outline_color_;
  QPointer<AlphaSlider> outline_alpha_;
  QPointer<QCheckBox> fill_visible_;
  QPointer<ColorButton> fill_color_;
  QPointer<AlphaSlider> fill_alpha_;
  QPointer<QDoubleSpinBox> line_width_;

  bool editing_{false};
  bool loading_{false};
};
#endif  // SHAPE_PANEL_HPP
