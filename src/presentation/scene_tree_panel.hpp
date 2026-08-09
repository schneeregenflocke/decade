#ifndef SCENE_TREE_PANEL_HPP
#define SCENE_TREE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cstddef>
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

// Presentation: master/detail view of the render scene graph. A collapsible
// tree on the left mirrors the GL-free SceneNodeSnapshot (delivered via the
// EventBus on every scene rebuild); a two-column list on the right shows the
// selected node's detail. For nodes bound to a domain ShapeConfiguration (via
// the snapshot's `style_id`) the detail also shows the node's colours and line
// width, looked up in the received ShapeConfigSet. The panel never touches the
// OpenGL `SceneNode` type, keeping the presentation layer graphics-free.
//
// The tree mirrors the scene, it does not edit it: **every** row is read-only.
// An edit field here would be a second write path onto state the rebuild
// recreates anyway; changes happen where the state is at home (the panel of the
// respective configuration).
class SceneTreePanel : public QWidget {
 public:
  explicit SceneTreePanel(QWidget* parent) : QWidget(parent) {
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

  void ReceiveSceneSnapshot(const SceneNodeSnapshot& snapshot) {
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
        stack.push_back(
            {.item = child_item, .node = &child, .path = child_path});
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

  // The set is the source of truth for the colours/line width shown in the
  // detail; keep a copy so a selected node's style stays in sync when the
  // configuration changes elsewhere.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set) {
    shape_config_set_ = shape_config_set;
    RefreshDetail();
  }

  // Emits the path of the currently selected node (nullopt when none) so the
  // renderer can highlight that node and its subtree on the calendar.
  [[nodiscard]] auto& SignalSelectedNode() { return signal_selected_node_; }

  // Selects the tree item at `path`, driving the normal selection path (detail
  // plus highlight emit). A debug/screenshot aid for exercising the panel
  // without a pointer device; a no-op when the path is unknown.
  void SelectNodeByPath(const std::string& path) {
    if (tree_ == nullptr) {
      return;
    }
    SelectItemAt(path);
  }

  // The selection came from outside — from a click in the canvas today. The
  // tree follows without reporting it back: otherwise the value would run in a
  // circle.
  void ReceiveSelectedNode(const std::optional<std::string>& path) {
    if (!path.has_value() || *path == SelectedPath()) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(rebuilding_);
    SelectNodeByPath(*path);
    RefreshDetail();
  }

 private:
  // Per-node payload: the snapshot node's scalar fields plus its child count. A
  // copy (not a reference into the snapshot) so it survives the snapshot being
  // replaced on the next rebuild. Keyed by the node's stable path, which is
  // also what the tree item carries — no pointer to an item is ever stored, and
  // the tree stays the single owner of its items.
  struct NodeDetail {
    SceneNodeValues values;
    std::size_t child_count{0};
  };

  static constexpr int kSashPositionPx = 220;
  static constexpr int kPathRole = Qt::UserRole;
  static constexpr int kLabelColumn = 0;
  static constexpr int kValueColumn = 1;

  void ApplyNode(QTreeWidgetItem* item, const SceneNodeSnapshot& node,
                 const std::string& path) {
    item->setText(kLabelColumn, MakeLabel(node));
    item->setData(kLabelColumn, kPathRole, QString::fromStdString(path));
    node_details_.insert_or_assign(
        path,
        NodeDetail{.values = node.values, .child_count = node.children.size()});
  }

  static QString MakeLabel(const SceneNodeSnapshot& node) {
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

  static QString ShapeKindLabel(SnapshotShapeKind kind) {
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

  // Boxes and sizes stand in page millimetres; two decimals suffice to see
  // differences of 0.1 mm.
  static QString Millimetres(float value) {
    return QString::number(static_cast<double>(value), 'f', 2);
  }

  static QString BoundsLabel(const SnapshotBounds& bounds) {
    return QString("l %1  r %2  b %3  t %4  (%5 x %6)")
        .arg(Millimetres(bounds.left), Millimetres(bounds.right),
             Millimetres(bounds.bottom), Millimetres(bounds.top),
             Millimetres(bounds.right - bounds.left),
             Millimetres(bounds.top - bounds.bottom));
  }

  // Walks the tree for the item carrying this path. Cheaper than it looks — the
  // alternative, a map of item pointers, would be a raw pointer member whose
  // entries the tree deletes underneath it on every rebuild.
  [[nodiscard]] QTreeWidgetItem* FindItemAt(const std::string& path) const {
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

  void SelectItemAt(const std::string& path) {
    if (QTreeWidgetItem* item = FindItemAt(path); item != nullptr) {
      tree_->setCurrentItem(item);
    }
  }

  void CallbackTreeSelectionChanged() {
    if (rebuilding_) {
      return;
    }
    RefreshDetail();
    EmitSelection();
  }

  // The stable path of the currently selected node, or empty when nothing
  // (valid) is selected. The single place that reads the selection's payload.
  [[nodiscard]] std::string SelectedPath() const {
    if (tree_ == nullptr) {
      return {};
    }
    const QTreeWidgetItem* selected = tree_->currentItem();
    if (selected == nullptr) {
      return {};
    }
    return selected->data(kLabelColumn, kPathRole).toString().toStdString();
  }

  // Publishes the selected node's stable path (or nullopt when the selection is
  // empty/invalid) on the selection signal.
  void EmitSelection() {
    const std::string path = SelectedPath();
    if (path.empty()) {
      signal_selected_node_(std::nullopt);
    } else {
      signal_selected_node_(std::optional<std::string>(path));
    }
  }

  // Rebuilds the detail from the current tree selection. Rebuilding wholesale
  // keeps the code simple and robust to the optional categories.
  void RefreshDetail() {
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
    AppendText(node_category, "Style ID",
               QString::fromStdString(node.style_id));

    AppendGeometryCategory(node);
    AppendTextCategory(node);
    AppendStyleCategory(node.style_id);

    detail_->expandAll();
    // After expanding, so the widest row counts: the property names are short
    // and fixed, the values are not, and a truncated "Outline …" says nothing.
    detail_->resizeColumnToContents(kLabelColumn);
  }

  // The node's own geometry: its box in node space and the same box on the
  // page. Containers without a shape have none.
  void AppendGeometryCategory(const SceneNodeValues& node) {
    if (!node.local_bounds.has_value() || !node.world_bounds.has_value()) {
      return;
    }
    QTreeWidgetItem* category = AppendCategory("Geometry");
    AppendText(category, "Local Bounds (mm)", BoundsLabel(*node.local_bounds));
    AppendText(category, "Page Bounds (mm)", BoundsLabel(*node.world_bounds));
  }

  // Text nodes additionally show what they draw and how large — in points,
  // because that is how a user reads font sizes, and in millimetres beside it,
  // because the page computes in those.
  void AppendTextCategory(const SceneNodeValues& node) {
    if (!node.text_detail.has_value()) {
      return;
    }
    QTreeWidgetItem* category = AppendCategory("Text");
    AppendText(category, "Content",
               QString::fromStdString(node.text_detail->text));
    AppendText(
        category, "Font Size (pt)",
        QString::number(static_cast<double>(domain::PointsFromMillimetres(
                            node.text_detail->size_millimetres)),
                        'f', 2));
    AppendText(
        category, "Font Size (mm)",
        QString::number(static_cast<double>(node.text_detail->size_millimetres),
                        'f', 2));
  }

  // If the node's style_id resolves to a configuration, show its colours, line
  // width and visibility flags — read-only, like everything here.
  void AppendStyleCategory(const std::string& style_id) {
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
    AppendText(category, "Fill Visible",
               config.FillVisible() ? "true" : "false");
    AppendColor(category, "Fill Color", config.FillColorDisabled());
    // The configured values, not those cleaned up by visibility: what stands
    // here should match what the user set.
    AppendText(category, "Line Width",
               QString::number(static_cast<double>(config.LineWidthDisabled()),
                               'f', 2));
  }

  // The categories are the top level of the detail list; every row hangs under
  // the one it was created with. The category travels as a parameter rather
  // than in a member — the list owns its items and deletes them on the next
  // rebuild, which is exactly the dangling a cached pointer would invite.
  QTreeWidgetItem* AppendCategory(const QString& label) {
    auto* category = MakeOwned<QTreeWidgetItem>(detail_.data());
    category->setText(kLabelColumn, label);
    QFont category_font = category->font(kLabelColumn);
    category_font.setBold(true);
    category->setFont(kLabelColumn, category_font);
    category->setFlags(Qt::ItemIsEnabled);
    return category;
  }

  // Every value stands as text: one kind of row, and no editable cell that
  // could write back on state the rebuild recreates anyway.
  static QTreeWidgetItem* AppendText(QTreeWidgetItem* category,
                                     const QString& label,
                                     const QString& value) {
    auto* row = MakeOwned<QTreeWidgetItem>(category);
    row->setText(kLabelColumn, label);
    row->setText(kValueColumn, value);
    row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    return row;
  }

  // Colours with alpha; the swatch shows the RGB, the numbers beside it name
  // all four channels — the swatch alone cannot show transparency.
  static void AppendColor(QTreeWidgetItem* category, const QString& label,
                          const glm::vec4& color) {
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

  QPointer<QTreeWidget> tree_;
  QPointer<QTreeWidget> detail_;
  std::unordered_map<std::string, NodeDetail> node_details_;
  ShapeConfigSet shape_config_set_;
  sigslot::signal<const std::optional<std::string>&> signal_selected_node_;

  bool rebuilding_{false};
};

#endif  // SCENE_TREE_PANEL_HPP
