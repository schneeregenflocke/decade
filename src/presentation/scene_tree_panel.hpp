#ifndef SCENE_TREE_PANEL_HPP
#define SCENE_TREE_PANEL_HPP

#include <wx/propgrid/advprops.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <memory>
#include <optional>
#include <sigslot/signal.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "../domain/scene_snapshot.hpp"
#include "../domain/shape_configuration.hpp"
#include "casts.hpp"
#include "rgba_colour_property.hpp"
#include "wx_owned.hpp"

// Presentation: master/detail view of the render scene graph. A collapsible
// tree on the left mirrors the GL-free SceneNodeSnapshot (delivered via the
// EventBus on every scene rebuild); a property grid on the right shows the
// selected node's detail. For nodes bound to a domain ShapeConfiguration (via
// the snapshot's `style_id`) the grid also shows the node's colours and line
// width, looked up in the received ShapeConfigSet. The panel never touches the
// OpenGL `SceneNode` type, keeping the presentation layer graphics-free.
//
// The node-info rows are read-only; the Style rows (colours, line width,
// visibility) are editable and committed back to the ShapeConfiguration store
// via SignalShapeConfigSet, so the edit survives the next scene rebuild.
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
    property_grid_->Bind(wxEVT_PG_CHANGED,
                         &SceneTreePanel::CallbackPropertyChanged, this);
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
    // happens after each property edit (edit -> scene rebuild -> snapshot) and
    // would force the user to re-select the node after every change.
    const std::string previously_selected = SelectedPath();
    tree_ctrl_->Freeze();
    tree_ctrl_->DeleteAllItems();
    path_to_item_.clear();
    const wxTreeItemId root = tree_ctrl_->AddRoot(MakeLabel(snapshot), -1, -1,
                                                  MakeItemData(snapshot, ""));
    path_to_item_.emplace(snapshot.name, root);

    // Iterative descent (no recursion): each frame pairs an already-created
    // tree item with the snapshot node whose children still need appending,
    // plus the path accumulated so far (a stable per-node identity).
    struct Frame {
      wxTreeItemId item;
      const SceneNodeSnapshot* node;
      std::string path;
    };
    std::vector<Frame> stack;
    stack.push_back({.item = root, .node = &snapshot, .path = snapshot.name});

    while (!stack.empty()) {
      const Frame frame = stack.back();
      stack.pop_back();
      for (const auto& child : frame.node->children) {
        const std::string child_path = frame.path + "/" + child.name;
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
    // Skip the grid rebuild when this update is the echo of our own edit: the
    // edit is committed inside a wxEVT_PG_CHANGED handler, and clearing the
    // grid there would destroy the property mid-event.
    if (!emitting_) {
      RefreshDetail();
    }
  }

  // Emits the path of the currently selected node (nullopt when none) so the
  // renderer can highlight that node and its subtree on the calendar.
  [[nodiscard]] auto& SignalSelectedNode() { return signal_selected_node_; }

  // Emits the edited configuration set when a Style property changes, so the
  // ShapeConfiguration store (and through it the renderer and the Layout panel)
  // pick up the change. Routing through the domain store is what makes the edit
  // survive the next scene rebuild.
  [[nodiscard]] auto& SignalShapeConfigSet() {
    return signal_shape_config_set_;
  }

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

 private:
  // Per-tree-item payload: the snapshot node's scalar fields plus its stable
  // path. A copy (not a pointer into the snapshot) so it survives the snapshot
  // being replaced on the next rebuild.
  class NodeData : public wxTreeItemData {
   public:
    NodeData(const SceneNodeSnapshot& node, std::string path)
        : name_(node.name),
          style_id_(node.style_id),
          path_(std::move(path)),
          shape_kind_(node.shape_kind),
          draw_layer_(node.draw_layer),
          child_count_(node.children.size()),
          has_shape_(node.has_shape) {}

    [[nodiscard]] const std::string& Name() const { return name_; }
    [[nodiscard]] const std::string& StyleId() const { return style_id_; }
    [[nodiscard]] const std::string& Path() const { return path_; }
    [[nodiscard]] SnapshotShapeKind ShapeKind() const { return shape_kind_; }
    [[nodiscard]] int DrawLayer() const { return draw_layer_; }
    [[nodiscard]] std::size_t ChildCount() const { return child_count_; }
    [[nodiscard]] bool HasShape() const { return has_shape_; }

   private:
    std::string name_;
    std::string style_id_;
    std::string path_;
    SnapshotShapeKind shape_kind_;
    int draw_layer_;
    std::size_t child_count_;
    bool has_shape_;
  };

  static NodeData* MakeItemData(const SceneNodeSnapshot& node,
                                const std::string& path) {
    return MakeOwned<NodeData>(node, path);
  }

  static wxString MakeLabel(const SceneNodeSnapshot& node) {
    wxString label = wxString::FromUTF8(node.name);
    if (label.empty()) {
      label = "(unnamed)";
    }
    // A node without a shape is a pure grouping/container node; mark it so the
    // tree distinguishes drawable nodes from structural ones at a glance.
    if (!node.has_shape) {
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
  // wholesale keeps the code simple and robust to the optional style category.
  void RefreshDetail() {
    if (property_grid_ == nullptr || tree_ctrl_ == nullptr) {
      return;
    }
    property_grid_->Clear();
    // The grid owns the recreated properties; we keep no pointers to them and
    // look the changed one up from the event instead. style_id_ names the
    // configuration the Style rows edit (empty when the node has no style).
    style_id_.clear();

    const wxTreeItemId selected = tree_ctrl_->GetSelection();
    if (!selected.IsOk()) {
      return;
    }
    const auto* data =
        dynamic_cast<NodeData*>(tree_ctrl_->GetItemData(selected));
    if (data == nullptr) {
      return;
    }

    property_grid_->Append(MakeOwned<wxPropertyCategory>("Node", wxPG_LABEL));
    AppendReadOnly(MakeOwned<wxStringProperty>(
        "Name", wxPG_LABEL, wxString::FromUTF8(data->Name())));
    AppendReadOnly(MakeOwned<wxStringProperty>(
        "Shape", wxPG_LABEL, ShapeKindLabel(data->ShapeKind())));
    AppendReadOnly(
        MakeOwned<wxIntProperty>("Draw Layer", wxPG_LABEL, data->DrawLayer()));
    AppendReadOnly(MakeOwned<wxIntProperty>(
        "Children", wxPG_LABEL, static_cast<int>(data->ChildCount())));
    AppendReadOnly(MakeOwned<wxStringProperty>(
        "Style ID", wxPG_LABEL, wxString::FromUTF8(data->StyleId())));

    AppendStyleCategory(data->StyleId());
  }

  // If the node's style_id resolves to a configuration, show its colours, line
  // width and visibility flags as editable properties. Edits are committed to
  // the domain ShapeConfiguration via CallbackPropertyChanged.
  void AppendStyleCategory(const std::string& style_id) {
    if (style_id.empty()) {
      return;
    }
    const ShapeConfiguration config =
        shape_config_set_.GetShapeConfiguration(style_id);
    if (config.Name() != style_id) {
      return;  // not found in the set
    }
    style_id_ = style_id;

    // Two colour widgets are shown side by side per colour, on purpose: the
    // RgbaColourProperty (custom, edits the alpha channel too) and the stock
    // wxColourProperty (RGB only). Editing either commits; CallbackProperty
    // Changed dispatches on the changed property so they do not clobber each
    // other, and the next rebuild re-syncs both from the configuration. Each
    // property is created with a stable name (used for lookup in the callback)
    // and is owned by the grid -- we deliberately keep no pointers to them.
    property_grid_->Append(MakeOwned<wxPropertyCategory>("Style", wxPG_LABEL));
    property_grid_->Append(MakeOwned<wxBoolProperty>(
        "Outline Visible", kOutlineVisibleName, config.OutlineVisible()));
    property_grid_->Append(MakeOwned<RgbaColourProperty>(
        "Outline Color (RGBA)", kOutlineColorRgbaName,
        ToWxColor(config.OutlineColorDisabled())));
    property_grid_->Append(
        MakeOwned<wxColourProperty>("Outline Color (RGB)", kOutlineColorRgbName,
                                    ToWxColor(config.OutlineColorDisabled())));
    property_grid_->Append(MakeOwned<wxBoolProperty>(
        "Fill Visible", kFillVisibleName, config.FillVisible()));
    property_grid_->Append(
        MakeOwned<RgbaColourProperty>("Fill Color (RGBA)", kFillColorRgbaName,
                                      ToWxColor(config.FillColorDisabled())));
    property_grid_->Append(
        MakeOwned<wxColourProperty>("Fill Color (RGB)", kFillColorRgbName,
                                    ToWxColor(config.FillColorDisabled())));
    property_grid_->Append(MakeOwned<wxFloatProperty>(
        "Line Width", kLineWidthName, config.LineWidthDisabled()));
  }

  void AppendReadOnly(wxPGProperty* property) {
    property_grid_->Append(property);
    property_grid_->SetPropertyReadOnly(property);
  }

  // Commits a Style-property edit into the configuration set and publishes it.
  // Only the property the user changed (carried by the event) is applied, so no
  // cached property handles are needed; the changed row is identified by its
  // stable name.
  void CallbackPropertyChanged(wxPropertyGridEvent& event) {
    if (style_id_.empty()) {
      return;
    }
    wxPGProperty* changed = event.GetProperty();
    if (changed == nullptr) {
      return;
    }
    ShapeConfigSet updated = shape_config_set_;
    ShapeConfiguration config = updated.GetShapeConfiguration(style_id_);
    if (config.Name() != style_id_) {
      return;  // configuration no longer present
    }
    if (!ApplyChange(changed, config)) {
      return;  // a non-Style (read-only Node) row, nothing to commit
    }
    updated.UpdateConfiguration(config);

    shape_config_set_ = updated;
    emitting_ = true;
    signal_shape_config_set_(updated);
    emitting_ = false;
  }

  // Applies the single changed Style row to `config`, identified by the
  // property's stable name. The RGBA colour row applies the full colour; the
  // RGB row applies red/green/blue while keeping the configured alpha. Returns
  // false when the changed property is not one of the editable Style rows.
  static bool ApplyChange(wxPGProperty* changed, ShapeConfiguration& config) {
    const wxString name = changed->GetName();
    if (name == kOutlineVisibleName) {
      config.OutlineVisible(changed->GetValue().GetBool());
    } else if (name == kFillVisibleName) {
      config.FillVisible(changed->GetValue().GetBool());
    } else if (name == kLineWidthName) {
      config.LineWidth(static_cast<float>(changed->GetValue().GetDouble()));
    } else if (name == kOutlineColorRgbaName) {
      config.OutlineColor(RgbaColorOf(changed));
    } else if (name == kFillColorRgbaName) {
      config.FillColor(RgbaColorOf(changed));
    } else if (name == kOutlineColorRgbName) {
      config.OutlineColor(
          RgbColorKeepingAlpha(changed, config.OutlineColorDisabled()));
    } else if (name == kFillColorRgbName) {
      config.FillColor(
          RgbColorKeepingAlpha(changed, config.FillColorDisabled()));
    } else {
      return false;
    }
    return true;
  }

  // Full RGBA from the custom colour property (blank when the cast fails).
  static glm::vec4 RgbaColorOf(wxPGProperty* property) {
    const auto* rgba = dynamic_cast<RgbaColourProperty*>(property);
    return rgba != nullptr ? ToGlmVec4(rgba->GetColour()) : glm::vec4{};
  }

  // RGB from the stock colour property, keeping `previous`' alpha.
  static glm::vec4 RgbColorKeepingAlpha(wxPGProperty* property,
                                        const glm::vec4& previous) {
    const auto* rgb = dynamic_cast<wxColourProperty*>(property);
    if (rgb == nullptr) {
      return previous;
    }
    glm::vec4 color = ToGlmVec4(rgb->GetVal().m_colour);
    color.a = previous.a;
    return color;
  }

  static constexpr int kSashPositionPx = 220;
  static constexpr int kMinPanePx = 80;

  // Stable property names for the editable Style rows, shared between creation
  // (AppendStyleCategory) and the change dispatch (ApplyChange). They are the
  // grid's lookup keys, so no property pointers need to be cached.
  static constexpr const char* kOutlineVisibleName = "outline_visible";
  static constexpr const char* kOutlineColorRgbaName = "outline_color_rgba";
  static constexpr const char* kOutlineColorRgbName = "outline_color_rgb";
  static constexpr const char* kFillVisibleName = "fill_visible";
  static constexpr const char* kFillColorRgbaName = "fill_color_rgba";
  static constexpr const char* kFillColorRgbName = "fill_color_rgb";
  static constexpr const char* kLineWidthName = "line_width";

  wxWeakRef<wxTreeCtrl> tree_ctrl_;
  wxWeakRef<wxPropertyGrid> property_grid_;
  std::unordered_map<std::string, wxTreeItemId> path_to_item_;
  ShapeConfigSet shape_config_set_;
  sigslot::signal<const std::optional<std::string>&> signal_selected_node_;
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;

  // Names the configuration the Style rows edit (empty when the selected node
  // has no style). The Style properties themselves are owned by the grid and
  // looked up by name, so they are not held here.
  std::string style_id_;

  bool rebuilding_{false};
  // True while emitting our own edit, to suppress the echo-driven grid rebuild.
  bool emitting_{false};
};

#endif  // SCENE_TREE_PANEL_HPP
