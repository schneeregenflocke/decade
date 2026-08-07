#ifndef SCENE_SNAPSHOT_HPP
#define SCENE_SNAPSHOT_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Kind of shape a scene node carries, as a GL-free enum so the presentation
// layer can describe a node without depending on the OpenGL shape types in
// `src/infrastructure/graphics/`.
enum class SnapshotShapeKind : std::uint8_t {
  kNone,
  kFill,
  kBoxes,
  kFont,
};

// An axis-aligned box in page millimetres. A type of its own instead of the
// RectF from `infrastructure/graphics/`, so the read model leaves no dependency
// there.
struct SnapshotBounds {
  float left{0.0F};
  float right{0.0F};
  float bottom{0.0F};
  float top{0.0F};
};

// What a text node shows beyond its content: the drawn text and the em size it
// was set at. Specific to the kind, not to the node — every font node carries
// it.
struct SnapshotTextDetail {
  std::string text;
  float size_millimetres{0.0F};
};

// Application-layer read model of the render scene graph: a plain, GL-free
// mirror of the SceneNode hierarchy. It exists so the presentation layer (the
// scene-tree widget) can display the graph structure and per-node detail
// without depending on the OpenGL `SceneNode` type in
// `src/infrastructure/graphics/`. The rendering side builds it; the bus carries
// it; the panel renders it.
//
// `style_id` is the name of the domain ShapeConfiguration the node's appearance
// derives from (empty when none). It is the link the detail pane uses to look
// up the node's colours/line width in the ShapeConfigSet.

// The values of a node without its children. A type of its own, because the two
// get used differently: the values travel on one by one (the tree hangs them
// onto its item) while the hierarchy merely gets walked. That way nobody copies
// a whole subtree along by accident.
struct SceneNodeValues {
  std::string name;
  std::string style_id;
  bool has_shape{false};
  SnapshotShapeKind shape_kind{SnapshotShapeKind::kNone};
  int draw_layer{0};
  // The box of its own geometry, in node space and in page coordinates; empty
  // when the node carries no geometry.
  std::optional<SnapshotBounds> local_bounds;
  std::optional<SnapshotBounds> world_bounds;
  std::optional<SnapshotTextDetail> text_detail;
};

struct SceneNodeSnapshot {
  SceneNodeValues values;
  std::vector<SceneNodeSnapshot> children;
};

#endif  // SCENE_SNAPSHOT_HPP
