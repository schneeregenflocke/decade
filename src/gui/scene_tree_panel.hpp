#ifndef SCENE_TREE_PANEL_HPP
#define SCENE_TREE_PANEL_HPP

#include <wx/treectrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <memory>
#include <vector>

#include "../app/binding/scene_snapshot.hpp"

// Presentation: read-only view of the render scene graph as a collapsible
// tree. It consumes a GL-free SceneNodeSnapshot (delivered via the EventBus on
// every scene rebuild) and never touches the OpenGL `SceneNode` type, keeping
// the presentation layer free of graphics dependencies.
class SceneTreePanel : public wxPanel {
 public:
  explicit SceneTreePanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr) {
    tree_ctrl_ = std::make_unique<wxTreeCtrl>(
                     this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     wxTR_DEFAULT_STYLE | wxTR_TWIST_BUTTONS)
                     .release();

    constexpr int kSizerBorderPx = 5;
    auto* vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    vertical_sizer->Add(tree_ctrl_, 1, wxEXPAND | wxALL, kSizerBorderPx);
    SetSizer(vertical_sizer);
  }

  void ReceiveSceneSnapshot(const SceneNodeSnapshot& snapshot) {
    if (tree_ctrl_ == nullptr) {
      return;
    }
    tree_ctrl_->Freeze();
    tree_ctrl_->DeleteAllItems();
    const wxTreeItemId root = tree_ctrl_->AddRoot(MakeLabel(snapshot));

    // Iterative descent (no recursion): each frame pairs an already-created
    // tree item with the snapshot node whose children still need appending.
    // Siblings are appended in order; only the descent order is LIFO, which
    // does not affect the visible ordering.
    struct Frame {
      wxTreeItemId item;
      const SceneNodeSnapshot* node;
    };
    std::vector<Frame> stack;
    stack.push_back({.item = root, .node = &snapshot});

    while (!stack.empty()) {
      const Frame frame = stack.back();
      stack.pop_back();
      for (const auto& child : frame.node->children) {
        const wxTreeItemId child_item =
            tree_ctrl_->AppendItem(frame.item, MakeLabel(child));
        stack.push_back({.item = child_item, .node = &child});
      }
    }

    tree_ctrl_->ExpandAll();
    tree_ctrl_->Thaw();
  }

 private:
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

  wxWeakRef<wxTreeCtrl> tree_ctrl_;
};

#endif  // SCENE_TREE_PANEL_HPP
