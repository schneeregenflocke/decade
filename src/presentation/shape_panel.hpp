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
#include <string>
#include <vector>

#include "../domain/date_category.hpp"
#include "../domain/detail/reentry_guard.hpp"
#include "../domain/shape_configuration.hpp"
#include "alpha_slider.hpp"
#include "casts.hpp"
#include "color_button.hpp"
#include "make_owned.hpp"

// The home of the shape configurations: the list on the left keys them, the
// fields on the right edit the selected one. The scene tree mirrors the same
// values and stays read-only — this is where they get changed (#65).
//
// Master and detail rather than one long form, because the set grows with the
// date categories: the fixed configurations plus one entry per category.
class ShapeSetupPanel : public QWidget {
  Q_OBJECT

 public:
  explicit ShapeSetupPanel(QWidget* parent);

  // The set arrives whole and replaces what stands here. The selection follows
  // the key, not the row: a new date category adds an entry and would otherwise
  // shift the selection to a different configuration.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set);

  // A category configuration reads as its category's name in the list.
  void ReceiveDateCategories(const std::vector<DateCategory>& date_categories);

 signals:
  void ShapeConfigSetEdited(const ShapeConfigSet& shape_config_set);

 private:
  static constexpr float kAlphaByteMax = 255.0F;
  static constexpr int kSashPositionPx = 180;
  static constexpr int kBorderPx = 5;
  static constexpr double kLineWidthMinMm = 0.0;
  static constexpr double kLineWidthMaxMm = 20.0;
  static constexpr double kLineWidthIncrementMm = 0.1;

  void CreateDetailFields(QWidget* detail_widget);

  // Every configuration of the set in one list: the fixed ones first, the
  // per-date-category ones after them, in the order the set holds them.
  [[nodiscard]] std::vector<std::string> ConfigurationKeys() const;

  void RebuildKeyList();

  // What the list shows for the configuration under `key`; the key itself
  // travels in the row's Qt::UserRole.
  [[nodiscard]] QString LabelFor(const std::string& key) const;

  static int RowOf(const std::vector<std::string>& keys,
                   const std::string& key);

  // The fields show the configured values, not the ones visibility cleans up:
  // switching a fill off and on again must give back the colour that was set.
  void RefreshDetail();

  static void ShowColor(const glm::vec4& color,
                        const QPointer<ColorButton>& button,
                        const QPointer<AlphaSlider>& alpha);

  static glm::vec4 ReadColor(const QPointer<ColorButton>& button,
                             const QPointer<AlphaSlider>& alpha);

  void CallbackSelection(int row);

  void CallbackEdit();

  ShapeConfigSet shape_config_set_;
  std::vector<std::string> category_names_;
  std::string selected_key_;

  QPointer<QListWidget> key_list_;
  QPointer<ColorButton> color_;
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
