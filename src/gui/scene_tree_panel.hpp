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
#include <vector>

#include "../app/binding/scene_snapshot.hpp"
#include "../packages/shape_configuration.hpp"
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
    tree_ctrl_->Freeze();
    tree_ctrl_->DeleteAllItems();
    const wxTreeItemId root = tree_ctrl_->AddRoot(MakeLabel(snapshot), -1, -1,
                                                  MakeItemData(snapshot, ""));

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
        stack.push_back(
            {.item = child_item, .node = &child, .path = child_path});
      }
    }

    tree_ctrl_->ExpandAll();
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

  // Publishes the selected node's stable path (or nullopt when the selection is
  // empty/invalid) on the selection signal.
  void EmitSelection() {
    if (tree_ctrl_ == nullptr) {
      return;
    }
    const wxTreeItemId selected = tree_ctrl_->GetSelection();
    if (selected.IsOk()) {
      const auto* data =
          dynamic_cast<NodeData*>(tree_ctrl_->GetItemData(selected));
      if (data != nullptr) {
        signal_selected_node_(std::optional<std::string>(data->Path()));
        return;
      }
    }
    signal_selected_node_(std::nullopt);
  }

  // Rebuilds the detail grid from the current tree selection. Rebuilding
  // wholesale keeps the code simple and robust to the optional style category.
  void RefreshDetail() {
    if (property_grid_ == nullptr || tree_ctrl_ == nullptr) {
      return;
    }
    property_grid_->Clear();
    // The editable style handles are recreated below (or left null when the
    // selected node has no configuration).
    style_id_.clear();
    outline_visible_property_ = nullptr;
    outline_color_property_ = nullptr;
    fill_visible_property_ = nullptr;
    fill_color_property_ = nullptr;
    line_width_property_ = nullptr;

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
        "Style", wxPG_LABEL, wxString::FromUTF8(data->StyleId())));

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

    property_grid_->Append(MakeOwned<wxPropertyCategory>("Style", wxPG_LABEL));
    outline_visible_property_ = MakeOwned<wxBoolProperty>(
        "Outline Visible", wxPG_LABEL, config.OutlineVisible());
    property_grid_->Append(outline_visible_property_);
    outline_color_property_ = MakeOwned<wxColourProperty>(
        "Outline Color", wxPG_LABEL, ToWxColor(config.OutlineColorDisabled()));
    property_grid_->Append(outline_color_property_);
    fill_visible_property_ = MakeOwned<wxBoolProperty>(
        "Fill Visible", wxPG_LABEL, config.FillVisible());
    property_grid_->Append(fill_visible_property_);
    fill_color_property_ = MakeOwned<wxColourProperty>(
        "Fill Color", wxPG_LABEL, ToWxColor(config.FillColorDisabled()));
    property_grid_->Append(fill_color_property_);
    line_width_property_ = MakeOwned<wxFloatProperty>(
        "Line Width", wxPG_LABEL, config.LineWidthDisabled());
    property_grid_->Append(line_width_property_);
  }

  void AppendReadOnly(wxPGProperty* property) {
    property_grid_->Append(property);
    property_grid_->SetPropertyReadOnly(property);
  }

  // Commits a Style-property edit into the configuration set and publishes it.
  // The RGB of the colour pickers is applied while the configured alpha is
  // preserved (the picker does not edit alpha), so transparency is not lost.
  void CallbackPropertyChanged(wxPropertyGridEvent& /*event*/) {
    if (style_id_.empty() || outline_color_property_ == nullptr) {
      return;
    }
    ShapeConfigSet updated = shape_config_set_;
    for (std::size_t index = 0; index < updated.size(); ++index) {
      if (updated[index].Name() != style_id_) {
        continue;
      }
      ShapeConfiguration& config = updated[index];

      config.OutlineVisible(outline_visible_property_->GetValue().GetBool());
      config.FillVisible(fill_visible_property_->GetValue().GetBool());
      config.LineWidth(
          static_cast<float>(line_width_property_->GetValue().GetDouble()));
      config.OutlineColor(ColorPreservingAlpha(outline_color_property_,
                                               config.OutlineColorDisabled()));
      config.FillColor(ColorPreservingAlpha(fill_color_property_,
                                            config.FillColorDisabled()));
      break;
    }

    shape_config_set_ = updated;
    emitting_ = true;
    signal_shape_config_set_(updated);
    emitting_ = false;
  }

  // RGB from the colour picker, alpha kept from the previous value.
  static glm::vec4 ColorPreservingAlpha(wxColourProperty* property,
                                        const glm::vec4& previous) {
    glm::vec4 color = ToGlmVec4(property->GetVal().m_colour);
    color.a = previous.a;
    return color;
  }

  static constexpr int kSashPositionPx = 220;
  static constexpr int kMinPanePx = 80;

  wxWeakRef<wxTreeCtrl> tree_ctrl_;
  wxWeakRef<wxPropertyGrid> property_grid_;
  ShapeConfigSet shape_config_set_;
  sigslot::signal<const std::optional<std::string>&> signal_selected_node_;
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;

  // Editable Style-category handles for the currently selected node, owned by
  // the property grid and recreated on every RefreshDetail(). style_id_ names
  // the configuration they edit (empty when the selection has no style).
  std::string style_id_;
  wxBoolProperty* outline_visible_property_{nullptr};
  wxColourProperty* outline_color_property_{nullptr};
  wxBoolProperty* fill_visible_property_{nullptr};
  wxColourProperty* fill_color_property_{nullptr};
  wxFloatProperty* line_width_property_{nullptr};

  bool rebuilding_{false};
  // True while emitting our own edit, to suppress the echo-driven grid rebuild.
  bool emitting_{false};
};

#endif  // SCENE_TREE_PANEL_HPP
