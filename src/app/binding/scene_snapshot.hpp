#ifndef SCENE_SNAPSHOT_HPP
#define SCENE_SNAPSHOT_HPP

#include <cstdint>
#include <string>
#include <vector>

// Kind of shape a scene node carries, as a GL-free enum so the presentation
// layer can describe a node without depending on the OpenGL shape types in
// `src/graphics/`.
enum class SnapshotShapeKind : std::uint8_t {
  kNone,
  kQuadrilateral,
  kRectangles,
  kFont,
};

// Application-layer read model of the render scene graph: a plain, GL-free
// mirror of the SceneNode hierarchy. It exists so the presentation layer (the
// scene-tree widget) can display the graph structure and per-node detail
// without depending on the OpenGL `SceneNode` type in `src/graphics/`. The
// rendering side builds it; the bus carries it; the panel renders it.
//
// `style_id` is the name of the domain ShapeConfiguration the node's appearance
// derives from (empty when none). It is the link the detail pane uses to look
// up the node's colours/line width in the ShapeConfigSet and to route an edit
// back to the right config.
struct SceneNodeSnapshot {
  std::string name;
  std::string style_id;
  bool has_shape{false};
  SnapshotShapeKind shape_kind{SnapshotShapeKind::kNone};
  int draw_layer{0};
  std::vector<SceneNodeSnapshot> children;
};

#endif  // SCENE_SNAPSHOT_HPP
