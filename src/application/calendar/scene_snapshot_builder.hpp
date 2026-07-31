#ifndef SCENE_SNAPSHOT_BUILDER_HPP
#define SCENE_SNAPSHOT_BUILDER_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "../../domain/scene_snapshot.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shapes.hpp"

// Classifies the shape carried by a node into the GL-free SnapshotShapeKind, so
// the read model can describe it without exposing the OpenGL shape types.
[[nodiscard]] inline SnapshotShapeKind ClassifyShape(const Shape* shape) {
  if (shape == nullptr) {
    return SnapshotShapeKind::kNone;
  }
  if (dynamic_cast<const QuadrilateralShape*>(shape) != nullptr) {
    return SnapshotShapeKind::kQuadrilateral;
  }
  if (dynamic_cast<const RectanglesShape*>(shape) != nullptr) {
    return SnapshotShapeKind::kRectangles;
  }
  if (dynamic_cast<const FontShape*>(shape) != nullptr) {
    return SnapshotShapeKind::kFont;
  }
  return SnapshotShapeKind::kNone;
}

[[nodiscard]] inline SnapshotBounds ToSnapshotBounds(const rectf& bounds) {
  return SnapshotBounds{.left = bounds.l(),
                        .right = bounds.r(),
                        .bottom = bounds.b(),
                        .top = bounds.t()};
}

// Die eigene Box des Knotens in Seitenkoordinaten. Alle vier Ecken werden
// transformiert und neu umschlossen, damit die Box auch dann stimmt, wenn eine
// Transformation je Achse unterschiedlich skaliert.
[[nodiscard]] inline SnapshotBounds ToWorldBounds(const rectf& local,
                                                  const glm::mat4& world) {
  const std::array<glm::vec4, 4> corners = {
      glm::vec4(local.l(), local.b(), 0.0F, 1.0F),
      glm::vec4(local.r(), local.b(), 0.0F, 1.0F),
      glm::vec4(local.l(), local.t(), 0.0F, 1.0F),
      glm::vec4(local.r(), local.t(), 0.0F, 1.0F)};

  const glm::vec4 first = world * corners[0];
  SnapshotBounds bounds{
      .left = first.x, .right = first.x, .bottom = first.y, .top = first.y};
  for (const auto& corner : corners) {
    const glm::vec4 transformed = world * corner;
    bounds.left = std::min(bounds.left, transformed.x);
    bounds.right = std::max(bounds.right, transformed.x);
    bounds.bottom = std::min(bounds.bottom, transformed.y);
    bounds.top = std::max(bounds.top, transformed.y);
  }
  return bounds;
}

// Der Text eines Font-Knotens; für jede andere Shape leer.
[[nodiscard]] inline std::optional<SnapshotTextDetail> TextDetailOf(
    const Shape* shape) {
  const auto* font_shape = dynamic_cast<const FontShape*>(shape);
  if (font_shape == nullptr) {
    return std::nullopt;
  }
  return SnapshotTextDetail{.text = font_shape->Text(),
                            .size_millimetres = font_shape->FontSize()};
}

// Fills a node's own values (everything but the children) from a scene node.
// `world` ist die aufsummierte Transformation bis einschliesslich dieses
// Knotens — dieselbe, mit der Draw() zeichnet.
inline void FillSnapshotValues(SceneNodeValues& destination,
                               const SceneNode& source,
                               const glm::mat4& world) {
  destination.name = source.GetNodeName();
  destination.style_id = source.GetStyleId();
  const Shape* shape = source.GetShape();
  destination.has_shape = shape != nullptr;
  destination.shape_kind = ClassifyShape(shape);
  destination.draw_layer = source.GetDrawLayer();
  destination.text_detail = TextDetailOf(shape);
  if (shape != nullptr) {
    const rectf& local = shape->LocalBounds();
    destination.local_bounds = ToSnapshotBounds(local);
    destination.world_bounds = ToWorldBounds(local, world);
  }
}

// Application/Infrastructure bridge: turns the live OpenGL `SceneNode` graph
// into the GL-free `SceneNodeSnapshot` read model consumed by the presentation
// layer. Kept apart from scene_snapshot.hpp (which must stay GL-free so the
// scene-tree panel never pulls in graphics headers) and out of the scene
// builder, whose job is building the graph, not mirroring it.
//
// Iterative tree copy (matching the scene graph's own non-recursive traversal
// style): each stack frame pairs a source SceneNode with the snapshot node it
// fills and the world transform accumulated down to it. Child vectors are sized
// once and never reallocated afterwards, so the stored destination pointers
// stay valid.
[[nodiscard]] inline SceneNodeSnapshot BuildSceneSnapshot(
    const SceneNode& root) {
  SceneNodeSnapshot result;
  const glm::mat4 root_world = root.GetModelMatrix();
  FillSnapshotValues(result.values, root, root_world);

  struct Frame {
    const SceneNode* source;
    SceneNodeSnapshot* destination;
    glm::mat4 world;
  };
  std::vector<Frame> stack;
  stack.push_back(
      {.source = &root, .destination = &result, .world = root_world});

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
      const glm::mat4 child_world =
          frame.world * visible[index]->GetModelMatrix();
      FillSnapshotValues(child.values, *visible[index], child_world);
      stack.push_back({.source = visible[index],
                       .destination = &child,
                       .world = child_world});
    }
  }

  return result;
}

#endif  // SCENE_SNAPSHOT_BUILDER_HPP
