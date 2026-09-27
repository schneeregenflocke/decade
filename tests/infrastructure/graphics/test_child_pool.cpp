#include <gtest/gtest.h>

#include <deque>
#include <glm/mat4x4.hpp>
#include <memory>
#include <string>
#include <vector>

#include "infrastructure/graphics/child_pool.hpp"
#include "infrastructure/graphics/drawable.hpp"
#include "infrastructure/graphics/rect.hpp"
#include "infrastructure/graphics/scene_graph.hpp"

// ShapeChildPool builds its shapes with a Shader and therefore needs a GL
// context; the ChildPool underneath it takes whatever builds a shape, so these
// tests run over drawables that touch no GL.

namespace {

template <DrawableKind kind>
class FakeShape : public Drawable {
 public:
  static constexpr DrawableKind kKind = kind;

  void Draw(const glm::mat4& /*model*/) const override {}

  [[nodiscard]] const RectF& LocalBounds() const override { return bounds_; }

  [[nodiscard]] DrawableKind Kind() const override { return kKind; }

 private:
  RectF bounds_;
};

using FakeBoxes = FakeShape<DrawableKind::kBoxes>;
using FakeText = FakeShape<DrawableKind::kText>;

constexpr int kLayer = 7;

TEST(ChildPoolTest, HandsBackTheSameNodeOnTheNextRebuild) {
  const auto parent = std::make_shared<SceneNode>("parent");

  SceneNode* first_pass = nullptr;
  {
    ChildPool pool(parent);
    first_pass = pool.Next("child").get();
  }

  {
    ChildPool pool(parent);
    EXPECT_EQ(pool.Next("child").get(), first_pass);
  }
}

TEST(ChildPoolTest, DropsTheChildrenAShorterSetNoLongerNeeds) {
  const auto parent = std::make_shared<SceneNode>("parent");

  {
    ChildPool pool(parent);
    (void)pool.Next("a");
    (void)pool.Next("b");
    (void)pool.Next("c");
  }
  EXPECT_EQ(parent->GetChildren().size(), 3U);

  {
    ChildPool pool(parent);
    (void)pool.Next("a");
  }
  EXPECT_EQ(parent->GetChildren().size(), 1U);
}

// The one that matters: a caller keeps what Next() handed back and asks for the
// next child afterwards. The parent's child vector grows in between and
// reallocates, so anything pointing into it moves. A pool that kept a reference
// to its parent — or a caller that kept a reference to a returned child — reads
// freed memory here.
//
// The shape this guards is `BuildBars`, which opens one child pool per date
// category and keeps them all alive while creating the next: with a single
// category nothing ever reallocates, so the fault appears the moment a second
// date category exists (#71).
TEST(ChildPoolTest, EarlierChildrenSurviveTheParentVectorGrowing) {
  const auto parent = std::make_shared<SceneNode>("parent");
  ChildPool pool(parent);

  std::vector<std::shared_ptr<SceneNode>> handed_out;
  constexpr std::size_t kEnoughToReallocate = 64;
  for (std::size_t index = 0; index < kEnoughToReallocate; ++index) {
    handed_out.push_back(pool.Next("child " + std::to_string(index)));
  }

  for (std::size_t index = 0; index < kEnoughToReallocate; ++index) {
    ASSERT_NE(handed_out[index], nullptr);
    EXPECT_EQ(handed_out[index]->GetNodeName(),
              "child " + std::to_string(index));
    EXPECT_EQ(handed_out[index], parent->GetChildren()[index]);
  }
}

// The same trap one level down, and the one the crash came out of: a pool built
// over a child that a *second* pool then makes the parent's vector grow past.
TEST(ChildPoolTest, ANestedPoolOutlivesItsParentVectorGrowing) {
  const auto root = std::make_shared<SceneNode>("root");
  ChildPool category_pool(root);

  // A deque, because the pool is neither copyable nor movable.
  std::deque<ChildPool> nested_pools;
  constexpr std::size_t kEnoughToReallocate = 64;
  for (std::size_t index = 0; index < kEnoughToReallocate; ++index) {
    nested_pools.emplace_back(
        category_pool.Next("category " + std::to_string(index)));
  }

  // Every nested pool still has a parent to fill, although the root's child
  // vector reallocated several times while they were being made.
  for (std::size_t index = 0; index < kEnoughToReallocate; ++index) {
    (void)nested_pools[index].Next("leaf");
  }

  for (std::size_t index = 0; index < kEnoughToReallocate; ++index) {
    ASSERT_EQ(root->GetChildren()[index]->GetChildren().size(), 1U);
    EXPECT_EQ(root->GetChildren()[index]->GetChildren()[0]->GetNodeName(),
              "leaf");
  }
}

// A date entry holds its bar pieces and their labels side by side, so one
// parent carries two kinds and a rebuild may ask for another kind at a
// position than the last one did.
TEST(ChildPoolTest, AChildKeepsItsShapeWhileItsKindStaysTheSame) {
  const auto parent = std::make_shared<SceneNode>("parent");

  const Drawable* first_pass = nullptr;
  {
    ChildPool pool(parent);
    first_pass = &pool.NextShape<FakeBoxes>("piece", kLayer).shape;
  }

  ChildPool pool(parent);
  EXPECT_EQ(&pool.NextShape<FakeBoxes>("piece", kLayer).shape, first_pass);
}

TEST(ChildPoolTest, AChildAskedForAnotherKindGetsANewShapeOnTheSameNode) {
  const auto parent = std::make_shared<SceneNode>("parent");

  SceneNode* first_node = nullptr;
  {
    ChildPool pool(parent);
    first_node = pool.NextShape<FakeBoxes>("piece", kLayer).node.get();
  }

  ChildPool pool(parent);
  const ShapeChild<FakeText> child = pool.NextShape<FakeText>("label", kLayer);
  EXPECT_EQ(child.node.get(), first_node);
  EXPECT_EQ(child.node->GetShape()->Kind(), DrawableKind::kText);
  EXPECT_EQ(child.node->GetDrawLayer(), kLayer);
}

TEST(ChildPoolTest, AGroupChildDropsTheShapeItDrewBefore) {
  const auto parent = std::make_shared<SceneNode>("parent");
  {
    ChildPool pool(parent);
    (void)pool.NextShape<FakeBoxes>("piece", kLayer);
  }

  ChildPool pool(parent);
  EXPECT_EQ(pool.Next("group")->GetShape(), nullptr);
}

}  // namespace
