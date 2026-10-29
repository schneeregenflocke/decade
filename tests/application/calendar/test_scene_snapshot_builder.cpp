#include <gtest/gtest.h>

#include <glm/mat4x4.hpp>
#include <memory>
#include <string>

#include "application/calendar/scene_snapshot_builder.hpp"
#include "domain/scene_snapshot.hpp"
#include "graphics/drawable.hpp"
#include "graphics/rect.hpp"
#include "graphics/scene_graph.hpp"

namespace {

// Draws nothing and touches no GL; the kind is all the builder asks of it.
class KindOnly : public Drawable {
 public:
  explicit KindOnly(DrawableKind kind) : kind_(kind) {}

  void Draw(const glm::mat4& /*model*/) const override {}

  [[nodiscard]] const RectF& LocalBounds() const override { return bounds_; }

  [[nodiscard]] DrawableKind Kind() const override { return kind_; }

 private:
  DrawableKind kind_;
  RectF bounds_;
};

std::shared_ptr<SceneNode> MakeNode(const std::string& name,
                                    DrawableKind kind) {
  return std::make_shared<SceneNode>(name, std::make_unique<KindOnly>(kind));
}

// root -> entry (a group styled "category 1") -> piece. No text node: the
// builder casts a text kind to FontShape, which needs a GL context.
SceneNodeSnapshot SnapshotOfAnEntry() {
  SceneNode root("root");
  const auto entry = std::make_shared<SceneNode>("entry 1");
  entry->SetStyleId("category 1");
  entry->AddChild(MakeNode("piece 0", DrawableKind::kBoxes));
  root.AddChild(entry);
  return BuildSceneSnapshot(root);
}

}  // namespace

TEST(SceneSnapshotBuilder, APieceShowsTheStyleItsEntryHandsDown) {
  const SceneNodeSnapshot snapshot = SnapshotOfAnEntry();
  const SceneNodeValues& piece = snapshot.children[0].children[0].values;

  EXPECT_EQ(piece.style_id, "category 1");
  EXPECT_EQ(piece.style_inherited_from, "entry 1");
}

TEST(SceneSnapshotBuilder, TheEntryShowsItsStyleAsItsOwn) {
  const SceneNodeSnapshot snapshot = SnapshotOfAnEntry();
  const SceneNodeValues& entry = snapshot.children[0].values;

  EXPECT_EQ(entry.style_id, "category 1");
  EXPECT_TRUE(entry.style_inherited_from.empty());
}
