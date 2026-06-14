#ifndef SCENE_SNAPSHOT_BUILDER_HPP
#define SCENE_SNAPSHOT_BUILDER_HPP

#include <cstddef>
#include <vector>

#include "../../graphics/scene_graph.hpp"
#include "scene_snapshot.hpp"

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
  result.name = root.GetNodeName();
  result.has_shape = root.GetShape() != nullptr;

  struct Frame {
    const SceneNode* source;
    SceneNodeSnapshot* destination;
  };
  std::vector<Frame> stack;
  stack.push_back({.source = &root, .destination = &result});

  while (!stack.empty()) {
    const Frame frame = stack.back();
    stack.pop_back();

    const auto& children = frame.source->GetChildren();
    frame.destination->children.resize(children.size());
    for (std::size_t index = 0; index < children.size(); ++index) {
      SceneNodeSnapshot& child = frame.destination->children[index];
      child.name = children[index]->GetNodeName();
      child.has_shape = children[index]->GetShape() != nullptr;
      stack.push_back({.source = children[index].get(), .destination = &child});
    }
  }

  return result;
}

#endif  // SCENE_SNAPSHOT_BUILDER_HPP
