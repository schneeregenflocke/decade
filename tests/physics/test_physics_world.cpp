#include <gtest/gtest.h>

#include <vector>

#include "graphics/pick_id.hpp"
#include "graphics/rect.hpp"
#include "physics/physics_world.hpp"

namespace {

std::vector<PhysicsWorld::PickBox> TwoBoxes() {
  return {
      {PickId{PickId::Kind::kBar, 0}, rectf(0.0F, 10.0F, 0.0F, 10.0F)},
      {PickId{PickId::Kind::kBar, 1}, rectf(20.0F, 30.0F, 0.0F, 10.0F)},
  };
}

}  // namespace

TEST(PhysicsWorldTest, RaycastInsideBoxReturnsItsPickId) {
  PhysicsWorld world;
  world.Rebuild(TwoBoxes());

  const auto hit_a = world.Raycast(glm::vec2(5.0F, 5.0F));
  ASSERT_TRUE(hit_a.has_value());
  EXPECT_EQ(hit_a->index, 0U);

  const auto hit_b = world.Raycast(glm::vec2(25.0F, 5.0F));
  ASSERT_TRUE(hit_b.has_value());
  EXPECT_EQ(hit_b->index, 1U);
}

TEST(PhysicsWorldTest, RaycastInGapMisses) {
  PhysicsWorld world;
  world.Rebuild(TwoBoxes());

  EXPECT_FALSE(world.Raycast(glm::vec2(15.0F, 5.0F)).has_value());
  EXPECT_FALSE(world.Raycast(glm::vec2(5.0F, 50.0F)).has_value());
}

TEST(PhysicsWorldTest, RebuildReplacesPreviousBoxes) {
  PhysicsWorld world;
  world.Rebuild(TwoBoxes());
  ASSERT_TRUE(world.Raycast(glm::vec2(5.0F, 5.0F)).has_value());

  world.Rebuild({});
  EXPECT_FALSE(world.Raycast(glm::vec2(5.0F, 5.0F)).has_value());

  world.Rebuild(
      {{PickId{PickId::Kind::kBar, 7}, rectf(0.0F, 4.0F, 0.0F, 4.0F)}});
  const auto hit = world.Raycast(glm::vec2(2.0F, 2.0F));
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(hit->index, 7U);
}

TEST(PhysicsWorldTest, EmptyWorldMisses) {
  const PhysicsWorld world;
  EXPECT_FALSE(world.Raycast(glm::vec2(0.0F, 0.0F)).has_value());
}
