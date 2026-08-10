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
  explicit SceneTreePanel(QWidget* parent);

  void ReceiveSceneSnapshot(const SceneNodeSnapshot& snapshot);

  // The set is the source of truth for the colours/line width shown in the
  // detail; keep a copy so a selected node's style stays in sync when the
  // configuration changes elsewhere.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set);

  // Emits the path of the currently selected node (nullopt when none) so the
  // renderer can highlight that node and its subtree on the calendar.
  // Defined here, and this member therefore stays in the header: a deduced
  // return type has to be visible where it is called.
  [[nodiscard]] auto& SignalSelectedNode() { return signal_selected_node_; }

  // Selects the tree item at `path`, driving the normal selection path (detail
  // plus highlight emit). A debug/screenshot aid for exercising the panel
  // without a pointer device; a no-op when the path is unknown.
  void SelectNodeByPath(const std::string& path);

  // The selection came from outside — from a click in the canvas today. The
  // tree follows without reporting it back: otherwise the value would run in a
  // circle.
  void ReceiveSelectedNode(const std::optional<std::string>& path);

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
                 const std::string& path);

  static QString MakeLabel(const SceneNodeSnapshot& node);

  static QString ShapeKindLabel(SnapshotShapeKind kind);

  // Boxes and sizes stand in page millimetres; two decimals suffice to see
  // differences of 0.1 mm.
  static QString Millimetres(float value);

  static QString BoundsLabel(const SnapshotBounds& bounds);

  // Walks the tree for the item carrying this path. Cheaper than it looks — the
  // alternative, a map of item pointers, would be a raw pointer member whose
  // entries the tree deletes underneath it on every rebuild.
  [[nodiscard]] QTreeWidgetItem* FindItemAt(const std::string& path) const;

  void SelectItemAt(const std::string& path);

  void CallbackTreeSelectionChanged();

  // The stable path of the currently selected node, or empty when nothing
  // (valid) is selected. The single place that reads the selection's payload.
  [[nodiscard]] std::string SelectedPath() const;

  // Publishes the selected node's stable path (or nullopt when the selection is
  // empty/invalid) on the selection signal.
  void EmitSelection();

  // Rebuilds the detail from the current tree selection. Rebuilding wholesale
  // keeps the code simple and robust to the optional categories.
  void RefreshDetail();

  // The node's own geometry: its box in node space and the same box on the
  // page. Containers without a shape have none.
  void AppendGeometryCategory(const SceneNodeValues& node);

  // Text nodes additionally show what they draw and how large — in points,
  // because that is how a user reads font sizes, and in millimetres beside it,
  // because the page computes in those.
  void AppendTextCategory(const SceneNodeValues& node);

  // If the node's style_id resolves to a configuration, show its colours, line
  // width and visibility flags — read-only, like everything here.
  void AppendStyleCategory(const std::string& style_id);

  // The categories are the top level of the detail list; every row hangs under
  // the one it was created with. The category travels as a parameter rather
  // than in a member — the list owns its items and deletes them on the next
  // rebuild, which is exactly the dangling a cached pointer would invite.
  QTreeWidgetItem* AppendCategory(const QString& label);

  // Every value stands as text: one kind of row, and no editable cell that
  // could write back on state the rebuild recreates anyway.
  static QTreeWidgetItem* AppendText(QTreeWidgetItem* category,
                                     const QString& label,
                                     const QString& value);

  // Colours with alpha; the swatch shows the RGB, the numbers beside it name
  // all four channels — the swatch alone cannot show transparency.
  static void AppendColor(QTreeWidgetItem* category, const QString& label,
                          const glm::vec4& color);

  QPointer<QTreeWidget> tree_;
  QPointer<QTreeWidget> detail_;
  std::unordered_map<std::string, NodeDetail> node_details_;
  ShapeConfigSet shape_config_set_;
  sigslot::signal<const std::optional<std::string>&> signal_selected_node_;

  bool rebuilding_{false};
};

#endif  // SCENE_TREE_PANEL_HPP
