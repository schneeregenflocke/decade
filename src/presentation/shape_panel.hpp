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
  explicit ShapeSetupPanel(QWidget* parent);

  // The set arrives whole and replaces what stands here. The selection follows
  // the name, not the row: a new date group adds an entry and would otherwise
  // shift the selection to a different configuration.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set);

  sigslot::signal<const ShapeConfigSet&>& SignalShapeConfigSet();

 private:
  static constexpr float kAlphaByteMax = 255.0F;
  static constexpr int kSashPositionPx = 180;
  static constexpr int kBorderPx = 5;
  static constexpr double kLineWidthMinMm = 0.0;
  static constexpr double kLineWidthMaxMm = 20.0;
  static constexpr double kLineWidthIncrementMm = 0.1;

  void CreateDetailFields(QWidget* detail_widget);

  // Every configuration of the set in one list: the fixed ones first, the
  // per-date-group ones after them, in the order the set holds them.
  [[nodiscard]] std::vector<std::string> ConfigurationNames() const;

  void RebuildNameList();

  static int RowOf(const std::vector<std::string>& names,
                   const std::string& name);

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
