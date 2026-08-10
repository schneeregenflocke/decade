#include "scene_snapshot_builder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <vector>

#include "../../domain/scene_snapshot.hpp"
#include "../../infrastructure/graphics/drawable.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"

SnapshotShapeKind ClassifyShape(const Drawable* shape) {
  if (shape == nullptr) {
    return SnapshotShapeKind::kNone;
  }
  switch (shape->Kind()) {
    case DrawableKind::kFill:
      return SnapshotShapeKind::kFill;
    case DrawableKind::kBoxes:
      return SnapshotShapeKind::kBoxes;
    case DrawableKind::kText:
      return SnapshotShapeKind::kFont;
    case DrawableKind::kNone:
      break;
  }
  return SnapshotShapeKind::kNone;
}

SnapshotBounds ToSnapshotBounds(const RectF& bounds) {
  return SnapshotBounds{.left = bounds.Left(),
                        .right = bounds.Right(),
                        .bottom = bounds.Bottom(),
                        .top = bounds.Top()};
}

SnapshotBounds ToWorldBounds(const RectF& local, const glm::mat4& world) {
  const std::array<glm::vec4, 4> corners = {
      glm::vec4(local.Left(), local.Bottom(), 0.0F, 1.0F),
      glm::vec4(local.Right(), local.Bottom(), 0.0F, 1.0F),
      glm::vec4(local.Left(), local.Top(), 0.0F, 1.0F),
      glm::vec4(local.Right(), local.Top(), 0.0F, 1.0F)};

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

std::optional<SnapshotTextDetail> TextDetailOf(const Drawable* shape) {
  if (shape == nullptr || shape->Kind() != DrawableKind::kText) {
    return std::nullopt;
  }
  const auto& font_shape = static_cast<const FontShape&>(*shape);
  return SnapshotTextDetail{.text = font_shape.Text(),
                            .size_millimetres = font_shape.FontSize()};
}

void FillSnapshotValues(SceneNodeValues& destination, const SceneNode& source,
                        const glm::mat4& world) {
  destination.name = source.GetNodeName();
  destination.style_id = source.GetStyleId();
  const Drawable* shape = source.GetShape();
  destination.has_shape = shape != nullptr;
  destination.shape_kind = ClassifyShape(shape);
  destination.draw_layer = source.GetDrawLayer();
  destination.text_detail = TextDetailOf(shape);
  if (shape != nullptr) {
    const RectF& local = shape->LocalBounds();
    destination.local_bounds = ToSnapshotBounds(local);
    destination.world_bounds = ToWorldBounds(local, world);
  }
}

SceneNodeSnapshot BuildSceneSnapshot(const SceneNode& root) {
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
