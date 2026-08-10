#include "scene_tree_panel.hpp"

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <sigslot/signal.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "../domain/detail/reentry_guard.hpp"
#include "../domain/scene_snapshot.hpp"
#include "../domain/shape_configuration.hpp"
#include "../domain/typography.hpp"
#include "casts.hpp"
#include "make_owned.hpp"

SceneTreePanel::SceneTreePanel(QWidget* parent) : QWidget(parent) {
  constexpr int kBorderPx = 5;

  auto* splitter = MakeOwned<QSplitter>(Qt::Horizontal, this);

  tree_ = MakeOwned<QTreeWidget>(splitter);
  tree_->setHeaderHidden(true);
  tree_->setColumnCount(1);

  detail_ = MakeOwned<QTreeWidget>(splitter);
  detail_->setColumnCount(2);
  detail_->setHeaderLabels({"Property", "Value"});
  detail_->setRootIsDecorated(false);

  splitter->addWidget(tree_);
  splitter->addWidget(detail_);
  splitter->setSizes({kSashPositionPx, kSashPositionPx});

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                      kBorderPx);
  vertical_layout->addWidget(splitter);
  setLayout(vertical_layout);

  connect(tree_.data(), &QTreeWidget::currentItemChanged, this,
          [this](QTreeWidgetItem*, QTreeWidgetItem*) {
            CallbackTreeSelectionChanged();
          });
}

void SceneTreePanel::ReceiveSceneSnapshot(const SceneNodeSnapshot& snapshot) {
  if (tree_ == nullptr) {
    return;
  }
  // Suppress selection events fired by the rebuild: the renderer keeps
  // highlighting the last selected path across rebuilds, so a spurious
  // "nothing selected" event here would wrongly clear it.
  rebuilding_ = true;
  // Remember the selected node's stable path so the rebuild can restore it.
  // Without this the selection collapses to the root on every rebuild, which
  // happens after each state change, and would force the user to re-select
  // the node afterwards.
  const std::string previously_selected = SelectedPath();
  tree_->setUpdatesEnabled(false);
  tree_->clear();
  node_details_.clear();

  auto* root = MakeOwned<QTreeWidgetItem>(tree_.data());
  ApplyNode(root, snapshot, snapshot.values.name);

  // Iterative descent (no recursion): each frame pairs an already-created
  // tree item with the snapshot node whose children still need appending,
  // plus the path accumulated so far (a stable per-node identity).
  struct Frame {
    QTreeWidgetItem* item;
    const SceneNodeSnapshot* node;
    std::string path;
  };
  std::vector<Frame> stack;
  stack.push_back(
      {.item = root, .node = &snapshot, .path = snapshot.values.name});

  while (!stack.empty()) {
    const Frame frame = stack.back();
    stack.pop_back();
    for (const auto& child : frame.node->children) {
      const std::string child_path = frame.path + "/" + child.values.name;
      auto* child_item = MakeOwned<QTreeWidgetItem>(frame.item);
      ApplyNode(child_item, child, child_path);
      stack.push_back({.item = child_item, .node = &child, .path = child_path});
    }
  }

  tree_->expandAll();

  // Restore the previous selection by path. Still under the rebuilding_ guard
  // so setting it does not re-emit the selection (which the renderer already
  // tracks); the detail is refreshed explicitly below.
  if (!previously_selected.empty()) {
    SelectItemAt(previously_selected);
  }

  tree_->setUpdatesEnabled(true);
  rebuilding_ = false;
  RefreshDetail();
}

void SceneTreePanel::ReceiveShapeConfigSet(
    const ShapeConfigSet& shape_config_set) {
  shape_config_set_ = shape_config_set;
  RefreshDetail();
}

void SceneTreePanel::SelectNodeByPath(const std::string& path) {
  if (tree_ == nullptr) {
    return;
  }
  SelectItemAt(path);
}

void SceneTreePanel::ReceiveSelectedNode(
    const std::optional<std::string>& path) {
  if (!path.has_value() || *path == SelectedPath()) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(rebuilding_);
  SelectNodeByPath(*path);
  RefreshDetail();
}

void SceneTreePanel::ApplyNode(QTreeWidgetItem* item,
                               const SceneNodeSnapshot& node,
                               const std::string& path) {
  item->setText(kLabelColumn, MakeLabel(node));
  item->setData(kLabelColumn, kPathRole, QString::fromStdString(path));
  node_details_.insert_or_assign(
      path,
      NodeDetail{.values = node.values, .child_count = node.children.size()});
}

QString SceneTreePanel::MakeLabel(const SceneNodeSnapshot& node) {
  QString label = QString::fromStdString(node.values.name);
  if (label.isEmpty()) {
    label = "(unnamed)";
  }
  // A node without a shape is a pure grouping/container node; mark it so the
  // tree distinguishes drawable nodes from structural ones at a glance.
  if (!node.values.has_shape) {
    label += "  •";
  }
  return label;
}

QString SceneTreePanel::ShapeKindLabel(SnapshotShapeKind kind) {
  switch (kind) {
    case SnapshotShapeKind::kFill:
      return "Fill";
    case SnapshotShapeKind::kBoxes:
      return "Boxes";
    case SnapshotShapeKind::kFont:
      return "Font";
    case SnapshotShapeKind::kNone:
      break;
  }
  return "(none)";
}

QString SceneTreePanel::Millimetres(float value) {
  return QString::number(static_cast<double>(value), 'f', 2);
}

QString SceneTreePanel::BoundsLabel(const SnapshotBounds& bounds) {
  return QString("l %1  r %2  b %3  t %4  (%5 x %6)")
      .arg(Millimetres(bounds.left), Millimetres(bounds.right),
           Millimetres(bounds.bottom), Millimetres(bounds.top),
           Millimetres(bounds.right - bounds.left),
           Millimetres(bounds.top - bounds.bottom));
}

QTreeWidgetItem* SceneTreePanel::FindItemAt(const std::string& path) const {
  const QString wanted = QString::fromStdString(path);
  std::vector<QTreeWidgetItem*> stack;
  stack.reserve(static_cast<std::size_t>(tree_->topLevelItemCount()));
  for (int index = 0; index < tree_->topLevelItemCount(); ++index) {
    stack.push_back(tree_->topLevelItem(index));
  }
  while (!stack.empty()) {
    QTreeWidgetItem* item = stack.back();
    stack.pop_back();
    if (item->data(kLabelColumn, kPathRole).toString() == wanted) {
      return item;
    }
    for (int index = 0; index < item->childCount(); ++index) {
      stack.push_back(item->child(index));
    }
  }
  return nullptr;
}

void SceneTreePanel::SelectItemAt(const std::string& path) {
  if (QTreeWidgetItem* item = FindItemAt(path); item != nullptr) {
    tree_->setCurrentItem(item);
  }
}

void SceneTreePanel::CallbackTreeSelectionChanged() {
  if (rebuilding_) {
    return;
  }
  RefreshDetail();
  EmitSelection();
}

std::string SceneTreePanel::SelectedPath() const {
  if (tree_ == nullptr) {
    return {};
  }
  const QTreeWidgetItem* selected = tree_->currentItem();
  if (selected == nullptr) {
    return {};
  }
  return selected->data(kLabelColumn, kPathRole).toString().toStdString();
}

void SceneTreePanel::EmitSelection() {
  const std::string path = SelectedPath();
  if (path.empty()) {
    signal_selected_node_(std::nullopt);
  } else {
    signal_selected_node_(std::optional<std::string>(path));
  }
}

void SceneTreePanel::RefreshDetail() {
  if (detail_ == nullptr || tree_ == nullptr) {
    return;
  }
  detail_->clear();

  const auto iterator = node_details_.find(SelectedPath());
  if (iterator == node_details_.end()) {
    return;
  }
  const NodeDetail& detail = iterator->second;
  const SceneNodeValues& node = detail.values;

  QTreeWidgetItem* node_category = AppendCategory("Node");
  AppendText(node_category, "Name", QString::fromStdString(node.name));
  AppendText(node_category, "Shape", ShapeKindLabel(node.shape_kind));
  AppendText(node_category, "Draw Layer", QString::number(node.draw_layer));
  AppendText(node_category, "Children", QString::number(detail.child_count));
  AppendText(node_category, "Style ID", QString::fromStdString(node.style_id));

  AppendGeometryCategory(node);
  AppendTextCategory(node);
  AppendStyleCategory(node.style_id);

  detail_->expandAll();
  // After expanding, so the widest row counts: the property names are short
  // and fixed, the values are not, and a truncated "Outline …" says nothing.
  detail_->resizeColumnToContents(kLabelColumn);
}

void SceneTreePanel::AppendGeometryCategory(const SceneNodeValues& node) {
  if (!node.local_bounds.has_value() || !node.world_bounds.has_value()) {
    return;
  }
  QTreeWidgetItem* category = AppendCategory("Geometry");
  AppendText(category, "Local Bounds (mm)", BoundsLabel(*node.local_bounds));
  AppendText(category, "Page Bounds (mm)", BoundsLabel(*node.world_bounds));
}

void SceneTreePanel::AppendTextCategory(const SceneNodeValues& node) {
  if (!node.text_detail.has_value()) {
    return;
  }
  QTreeWidgetItem* category = AppendCategory("Text");
  AppendText(category, "Content",
             QString::fromStdString(node.text_detail->text));
  AppendText(category, "Font Size (pt)",
             QString::number(static_cast<double>(domain::PointsFromMillimetres(
                                 node.text_detail->size_millimetres)),
                             'f', 2));
  AppendText(
      category, "Font Size (mm)",
      QString::number(static_cast<double>(node.text_detail->size_millimetres),
                      'f', 2));
}

void SceneTreePanel::AppendStyleCategory(const std::string& style_id) {
  if (style_id.empty()) {
    return;
  }
  const ShapeConfiguration config =
      shape_config_set_.GetShapeConfiguration(style_id);
  if (config.Name() != style_id) {
    return;  // not found in the set
  }

  QTreeWidgetItem* category = AppendCategory("Style");
  AppendText(category, "Outline Visible",
             config.OutlineVisible() ? "true" : "false");
  AppendColor(category, "Outline Color", config.OutlineColorDisabled());
  AppendText(category, "Fill Visible", config.FillVisible() ? "true" : "false");
  AppendColor(category, "Fill Color", config.FillColorDisabled());
  // The configured values, not those cleaned up by visibility: what stands
  // here should match what the user set.
  AppendText(
      category, "Line Width",
      QString::number(static_cast<double>(config.LineWidthDisabled()), 'f', 2));
}

QTreeWidgetItem* SceneTreePanel::AppendCategory(const QString& label) {
  auto* category = MakeOwned<QTreeWidgetItem>(detail_.data());
  category->setText(kLabelColumn, label);
  QFont category_font = category->font(kLabelColumn);
  category_font.setBold(true);
  category->setFont(kLabelColumn, category_font);
  category->setFlags(Qt::ItemIsEnabled);
  return category;
}

QTreeWidgetItem* SceneTreePanel::AppendText(QTreeWidgetItem* category,
                                            const QString& label,
                                            const QString& value) {
  auto* row = MakeOwned<QTreeWidgetItem>(category);
  row->setText(kLabelColumn, label);
  row->setText(kValueColumn, value);
  row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  return row;
}

void SceneTreePanel::AppendColor(QTreeWidgetItem* category,
                                 const QString& label, const glm::vec4& color) {
  const QColor qt_color = ToQColor(color);
  QTreeWidgetItem* row =
      AppendText(category, label, qt_color.name(QColor::HexRgb));
  row->setData(kValueColumn, Qt::DecorationRole,
               QColor(qt_color.red(), qt_color.green(), qt_color.blue()));
  AppendText(category, label + " (RGBA)",
             QString("%1, %2, %3, %4")
                 .arg(qt_color.red())
                 .arg(qt_color.green())
                 .arg(qt_color.blue())
                 .arg(qt_color.alpha()));
}
