#ifndef SCENE_TREE_PANEL_HPP
#define SCENE_TREE_PANEL_HPP

#include <wx/propgrid/advprops.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <cstddef>
#include <memory>
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
#include "wx_owned.hpp"

// Presentation: master/detail view of the render scene graph. A collapsible
// tree on the left mirrors the GL-free SceneNodeSnapshot (delivered via the
// EventBus on every scene rebuild); a property grid on the right shows the
// selected node's detail. For nodes bound to a domain ShapeConfiguration (via
// the snapshot's `style_id`) the grid also shows the node's colours and line
// width, looked up in the received ShapeConfigSet. The panel never touches the
// OpenGL `SceneNode` type, keeping the presentation layer graphics-free.
//
// The tree mirrors the scene, it does not edit it: **every** row is read-only.
// An edit field here would be a second write path onto state the rebuild
// recreates anyway; changes happen where the state is at home (the panel of the
// respective configuration).
class SceneTreePanel : public wxPanel {
 public:
  explicit SceneTreePanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr) {
    auto* splitter = MakeOwned<wxSplitterWindow>(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
    splitter->SetMinimumPaneSize(kMinPanePx);

    tree_ctrl_ = MakeOwned<wxTreeCtrl>(splitter, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTR_DEFAULT_STYLE | wxTR_TWIST_BUTTONS);
    property_grid_ =
        MakeOwned<wxPropertyGrid>(splitter, wxID_ANY, wxDefaultPosition,
                                  wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER);

    splitter->SplitVertically(tree_ctrl_, property_grid_, kSashPositionPx);

    constexpr int kSizerBorderPx = 5;
    auto* vertical_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    vertical_sizer->Add(splitter, 1, wxEXPAND | wxALL, kSizerBorderPx);
    SetSizer(vertical_sizer);

    tree_ctrl_->Bind(wxEVT_TREE_SEL_CHANGED,
                     &SceneTreePanel::CallbackTreeSelectionChanged, this);
  }

  void ReceiveSceneSnapshot(const SceneNodeSnapshot& snapshot) {
    if (tree_ctrl_ == nullptr) {
      return;
    }
    // Suppress selection events fired by DeleteAllItems/rebuild: the renderer
    // keeps highlighting the last selected path across rebuilds, so a spurious
    // "nothing selected" event here would wrongly clear it.
    rebuilding_ = true;
    // Remember the selected node's stable path so the rebuild can restore it.
    // Without this the selection collapses to the root on every rebuild, which
    // happens after each state change, and would force the user to re-select
    // the node afterwards.
    const std::string previously_selected = SelectedPath();
    tree_ctrl_->Freeze();
    tree_ctrl_->DeleteAllItems();
    path_to_item_.clear();
    const wxTreeItemId root = tree_ctrl_->AddRoot(MakeLabel(snapshot), -1, -1,
                                                  MakeItemData(snapshot, ""));
    path_to_item_.emplace(snapshot.values.name, root);

    // Iterative descent (no recursion): each frame pairs an already-created
    // tree item with the snapshot node whose children still need appending,
    // plus the path accumulated so far (a stable per-node identity).
    struct Frame {
      wxTreeItemId item;
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
        const wxTreeItemId child_item =
            tree_ctrl_->AppendItem(frame.item, MakeLabel(child), -1, -1,
                                   MakeItemData(child, child_path));
        path_to_item_.emplace(child_path, child_item);
        stack.push_back(
            {.item = child_item, .node = &child, .path = child_path});
      }
    }

    tree_ctrl_->ExpandAll();

    // Restore the previous selection by path. Still under the rebuilding_ guard
    // so SelectItem does not re-emit the selection (which the renderer already
    // tracks); the detail grid is refreshed explicitly below.
    if (!previously_selected.empty()) {
      const auto iterator = path_to_item_.find(previously_selected);
      if (iterator != path_to_item_.end()) {
        tree_ctrl_->SelectItem(iterator->second);
      }
    }

    tree_ctrl_->Thaw();
    rebuilding_ = false;
    RefreshDetail();
  }

  // The set is the source of truth for the colours/line width shown in the
  // detail grid; keep a copy so a selected node's style stays in sync when the
  // configuration changes elsewhere.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set) {
    shape_config_set_ = shape_config_set;
    RefreshDetail();
  }

  // Emits the path of the currently selected node (nullopt when none) so the
  // renderer can highlight that node and its subtree on the calendar.
  [[nodiscard]] auto& SignalSelectedNode() { return signal_selected_node_; }

  // Selects the tree item at `path`, driving the normal selection path
  // (detail grid + highlight emit). A debug/screenshot aid for exercising the
  // panel without a pointer device; a no-op when the path is unknown.
  void SelectNodeByPath(const std::string& path) {
    if (tree_ctrl_ == nullptr) {
      return;
    }
    const auto iterator = path_to_item_.find(path);
    if (iterator != path_to_item_.end()) {
      tree_ctrl_->SelectItem(iterator->second);
    }
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
  // Per-tree-item payload: the snapshot node's scalar fields plus its stable
  // path. A copy (not a pointer into the snapshot) so it survives the snapshot
  // being replaced on the next rebuild.
  class NodeData : public wxTreeItemData {
   public:
    NodeData(const SceneNodeSnapshot& node, std::string path)
        : values_(node.values),
          path_(std::move(path)),
          child_count_(node.children.size()) {}

    [[nodiscard]] const SceneNodeValues& Values() const { return values_; }
    [[nodiscard]] const std::string& Path() const { return path_; }
    [[nodiscard]] std::size_t ChildCount() const { return child_count_; }

   private:
    SceneNodeValues values_;
    std::string path_;
    std::size_t child_count_;
  };

  static NodeData* MakeItemData(const SceneNodeSnapshot& node,
                                const std::string& path) {
    return MakeOwned<NodeData>(node, path);
  }

  static wxString MakeLabel(const SceneNodeSnapshot& node) {
    wxString label = wxString::FromUTF8(node.values.name);
    if (label.empty()) {
      label = "(unnamed)";
    }
    // A node without a shape is a pure grouping/container node; mark it so the
    // tree distinguishes drawable nodes from structural ones at a glance.
    if (!node.values.has_shape) {
      label << "  •";
    }
    return label;
  }

  static wxString ShapeKindLabel(SnapshotShapeKind kind) {
    switch (kind) {
      case SnapshotShapeKind::kQuadrilateral:
        return "Quadrilateral";
      case SnapshotShapeKind::kRectangles:
        return "Rectangles";
      case SnapshotShapeKind::kFont:
        return "Font";
      case SnapshotShapeKind::kNone:
        break;
    }
    return "(none)";
  }

  // Boxes and sizes stand in page millimetres; two decimals suffice to see
  // differences of 0.1 mm.
  static wxString BoundsLabel(const SnapshotBounds& bounds) {
    return wxString::Format("l %.2f  r %.2f  b %.2f  t %.2f  (%.2f x %.2f)",
                            bounds.left, bounds.right, bounds.bottom,
                            bounds.top, bounds.right - bounds.left,
                            bounds.top - bounds.bottom);
  }

  void CallbackTreeSelectionChanged(wxTreeEvent& /*event*/) {
    if (rebuilding_) {
      return;
    }
    RefreshDetail();
    EmitSelection();
  }

  // The stable path of the currently selected node, or empty when nothing
  // (valid) is selected. The single place that reads the selection's payload.
  [[nodiscard]] std::string SelectedPath() const {
    if (tree_ctrl_ == nullptr) {
      return {};
    }
    const wxTreeItemId selected = tree_ctrl_->GetSelection();
    if (!selected.IsOk()) {
      return {};
    }
    const auto* data =
        dynamic_cast<NodeData*>(tree_ctrl_->GetItemData(selected));
    return data != nullptr ? data->Path() : std::string{};
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

  // Rebuilds the detail grid from the current tree selection. Rebuilding
  // wholesale keeps the code simple and robust to the optional categories.
  void RefreshDetail() {
    if (property_grid_ == nullptr || tree_ctrl_ == nullptr) {
      return;
    }
    property_grid_->Clear();

    const wxTreeItemId selected = tree_ctrl_->GetSelection();
    if (!selected.IsOk()) {
      return;
    }
    const auto* data =
        dynamic_cast<NodeData*>(tree_ctrl_->GetItemData(selected));
    if (data == nullptr) {
      return;
    }
    const SceneNodeValues& node = data->Values();

    AppendCategory("Node");
    AppendText("Name", wxString::FromUTF8(node.name));
    AppendText("Shape", ShapeKindLabel(node.shape_kind));
    AppendInt("Draw Layer", node.draw_layer);
    AppendInt("Children", static_cast<int>(data->ChildCount()));
    AppendText("Style ID", wxString::FromUTF8(node.style_id));

    AppendGeometryCategory(node);
    AppendTextCategory(node);
    AppendStyleCategory(node.style_id);
  }

  // The node's own geometry: its box in node space and the same box on the
  // page. Containers without a shape have none.
  void AppendGeometryCategory(const SceneNodeValues& node) {
    if (!node.local_bounds.has_value() || !node.world_bounds.has_value()) {
      return;
    }
    AppendCategory("Geometry");
    AppendText("Local Bounds (mm)", BoundsLabel(*node.local_bounds));
    AppendText("Page Bounds (mm)", BoundsLabel(*node.world_bounds));
  }

  // Text nodes additionally show what they draw and how large — in points,
  // because that is how a user reads font sizes, and in millimetres beside it,
  // because the page computes in those.
  void AppendTextCategory(const SceneNodeValues& node) {
    if (!node.text_detail.has_value()) {
      return;
    }
    AppendCategory("Text");
    AppendText("Text", wxString::FromUTF8(node.text_detail->text));
    AppendText(
        "Font Size (pt)",
        wxString::Format("%.2f", domain::PointsFromMillimetres(
                                     node.text_detail->size_millimetres)));
    AppendText("Font Size (mm)",
               wxString::Format("%.2f", node.text_detail->size_millimetres));
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

    AppendCategory("Style");
    AppendText("Outline Visible", config.OutlineVisible() ? "true" : "false");
    AppendColor("Outline Color", config.OutlineColorDisabled());
    AppendText("Fill Visible", config.FillVisible() ? "true" : "false");
    AppendColor("Fill Color", config.FillColorDisabled());
    // The configured values, not those cleaned up by visibility: what stands
    // here should match what the user set.
    AppendText("Line Width",
               wxString::Format("%.2f", config.LineWidthDisabled()));
  }

  void AppendCategory(const wxString& label) {
    property_grid_->Append(MakeOwned<wxPropertyCategory>(label, wxPG_LABEL));
  }

  // Every value stands as text: one kind of row, no typed editors that could
  // write after all by accident.
  void AppendText(const wxString& label, const wxString& value) {
    AppendReadOnly(MakeOwned<wxStringProperty>(label, wxPG_LABEL, value));
  }

  void AppendInt(const wxString& label, int value) {
    AppendReadOnly(MakeOwned<wxIntProperty>(label, wxPG_LABEL, value));
  }

  // Colours with alpha; the wx colour picker knows no alpha channel, hence the
  // four channels as numbers plus the colour as a field beside them.
  void AppendColor(const wxString& label, const glm::vec4& color) {
    const wxColour wx_color = ToWxColor(color);
    AppendReadOnly(MakeOwned<wxColourProperty>(label, wxPG_LABEL, wx_color));
    AppendText(
        label + " (RGBA)",
        wxString::Format("%u, %u, %u, %u", wx_color.Red(), wx_color.Green(),
                         wx_color.Blue(), wx_color.Alpha()));
  }

  void AppendReadOnly(wxPGProperty* property) {
    property_grid_->Append(property);
    property_grid_->SetPropertyReadOnly(property);
  }

  static constexpr int kSashPositionPx = 220;
  static constexpr int kMinPanePx = 80;

  wxWeakRef<wxTreeCtrl> tree_ctrl_;
  wxWeakRef<wxPropertyGrid> property_grid_;
  std::unordered_map<std::string, wxTreeItemId> path_to_item_;
  ShapeConfigSet shape_config_set_;
  sigslot::signal<const std::optional<std::string>&> signal_selected_node_;

  bool rebuilding_{false};
};

#endif  // SCENE_TREE_PANEL_HPP
