#ifndef SCENE_SNAPSHOT_BUILDER_HPP
#define SCENE_SNAPSHOT_BUILDER_HPP

#include <glm/mat4x4.hpp>
#include <optional>

#include "../../domain/scene_snapshot.hpp"
#include "../../infrastructure/graphics/drawable.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"

// Translates what the drawable says it is into the GL-free SnapshotShapeKind,
// so the read model can describe it without exposing the OpenGL shape types.
// Two enums for one thing on purpose: the infrastructure names its own kinds,
// the read model belongs to the domain, and neither has to include the other.
[[nodiscard]] SnapshotShapeKind ClassifyShape(const Drawable* shape);

[[nodiscard]] SnapshotBounds ToSnapshotBounds(const RectF& bounds);

// The node's own box in page coordinates. All four corners get transformed and
// re-enclosed, so the box holds even when a transformation scales differently
// per axis.
[[nodiscard]] SnapshotBounds ToWorldBounds(const RectF& local,
                                           const glm::mat4& world);

// The text of a font node; empty for every other shape. The kind decides, and
// the cast then follows from it rather than testing for it.
[[nodiscard]] std::optional<SnapshotTextDetail> TextDetailOf(
    const Drawable* shape);

// Fills a node's own values (everything but the children) from a scene node.
// `world` is the accumulated transformation down to and including this node —
// the same one Draw() draws with.
void FillSnapshotValues(SceneNodeValues& destination, const SceneNode& source,
                        const glm::mat4& world);

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
[[nodiscard]] SceneNodeSnapshot BuildSceneSnapshot(const SceneNode& root);

#endif  // SCENE_SNAPSHOT_BUILDER_HPP
