#ifndef SCENE_SNAPSHOT_BUILDER_HPP
#define SCENE_SNAPSHOT_BUILDER_HPP

#include <cstddef>
#include <memory>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/scene_graph.hpp"
#include "../../graphics/shapes.hpp"
#include "scene_snapshot.hpp"

// Classifies the shape carried by a node into the GL-free SnapshotShapeKind, so
// the read model can describe it without exposing the OpenGL shape types.
[[nodiscard]] inline SnapshotShapeKind ClassifyShape(
    const std::shared_ptr<Shape>& shape) {
  if (shape == nullptr) {
    return SnapshotShapeKind::kNone;
  }
  if (std::dynamic_pointer_cast<QuadrilateralShape>(shape) != nullptr) {
    return SnapshotShapeKind::kQuadrilateral;
  }
  if (std::dynamic_pointer_cast<RectanglesShape>(shape) != nullptr) {
    return SnapshotShapeKind::kRectangles;
  }
  if (std::dynamic_pointer_cast<FontShape>(shape) != nullptr) {
    return SnapshotShapeKind::kFont;
  }
  return SnapshotShapeKind::kNone;
}

// Fills the scalar (non-children) fields of a snapshot node from a scene node.
inline void FillSnapshotFields(SceneNodeSnapshot& destination,
                               const SceneNode& source) {
  destination.name = source.GetNodeName();
  destination.style_id = source.GetStyleId();
  destination.has_shape = source.GetShape() != nullptr;
  destination.shape_kind = ClassifyShape(source.GetShape());
  destination.draw_layer = source.GetDrawLayer();
}

// Application/Infrastructure bridge: turns the live OpenGL `SceneNode` graph
// into the GL-free `SceneNodeSnapshot` read model consumed by the presentation
// layer. Kept apart from scene_snapshot.hpp (which must stay GL-free so the
// scene-tree panel never pulls in graphics headers) and out of the scene
// builder, whose job is building the graph, not mirroring it.
//
// Iterative tree copy (matching the scene graph's own non-recursive traversal
// style): each stack frame pairs a source SceneNode with the snapshot node it
// fills. Child vectors are sized once and never reallocated afterwards, so the
// stored destination pointers stay valid.
[[nodiscard]] inline SceneNodeSnapshot BuildSceneSnapshot(
    const SceneNode& root) {
  SceneNodeSnapshot result;
  FillSnapshotFields(result, root);

  struct Frame {
    const SceneNode* source;
    SceneNodeSnapshot* destination;
  };
  std::vector<Frame> stack;
  stack.push_back({.source = &root, .destination = &result});

  while (!stack.empty()) {
    const Frame frame = stack.back();
    stack.pop_back();

    // Internal rendering aids (e.g. the selection overlay) are excluded so the
    // user-facing tree mirrors only the real scene.
    std::vector<const SceneNode*> visible;
    for (const auto& child : frame.source->GetChildren()) {
      if (!child->IsSnapshotHidden()) {
        visible.push_back(child.get());
      }
    }
    frame.destination->children.resize(visible.size());
    for (std::size_t index = 0; index < visible.size(); ++index) {
      SceneNodeSnapshot& child = frame.destination->children[index];
      FillSnapshotFields(child, *visible[index]);
      stack.push_back({.source = visible[index], .destination = &child});
    }
  }

  return result;
}

#endif  // SCENE_SNAPSHOT_BUILDER_HPP
