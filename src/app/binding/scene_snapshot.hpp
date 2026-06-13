#ifndef SCENE_SNAPSHOT_HPP
#define SCENE_SNAPSHOT_HPP

#include <string>
#include <vector>

// Application-layer read model of the render scene graph: a plain, GL-free
// mirror of the SceneNode hierarchy (name + whether the node carries a shape +
// children). It exists so the presentation layer (the scene-tree widget) can
// display the graph structure without depending on the OpenGL `SceneNode` type
// in `src/graphics/`. The rendering side builds it; the bus carries it; the
// panel renders it.
struct SceneNodeSnapshot {
  std::string name;
  bool has_shape{false};
  std::vector<SceneNodeSnapshot> children;
};

#endif  // SCENE_SNAPSHOT_HPP
