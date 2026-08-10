#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include <btBulletCollisionCommon.h>

#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "../graphics/pick_id.hpp"

// Infrastructure: a thin RAII wrapper around a Bullet btCollisionWorld used for
// 2D hit-testing (picking). Each pickable element is registered as a thin,
// axis-aligned box centred at z = 0; casting a ray along z through the cursor's
// page-space point reports which element (by PickId) it hits. This collision
// world is the spatial structure that a later dynamics world (dragging, real
// physics) will extend without changing the picking path.
class PhysicsWorld {
 public:
  PhysicsWorld();

  // Replaces all registered pickables with the given boxes.
  void Rebuild(const std::vector<PickBox>& boxes);

  // Returns the PickId of the element under the given page-space point, if any.
  [[nodiscard]] std::optional<PickId> Raycast(glm::vec2 page_point) const;

 private:
  static constexpr float kHalf = 0.5F;
  static constexpr float kBoxHalfDepth = 1.0F;
  static constexpr float kRayHalfLength = 10.0F;

  // Declaration order matters for teardown: collision_world_ is declared last
  // so it is destroyed *first* — while the collision objects, shapes and the
  // broadphase it references are still alive.
  std::unique_ptr<btCollisionConfiguration> collision_configuration_;
  std::unique_ptr<btCollisionDispatcher> dispatcher_;
  std::unique_ptr<btBroadphaseInterface> broadphase_;
  std::vector<std::unique_ptr<btCollisionShape>> shapes_;
  std::vector<std::unique_ptr<btCollisionObject>> objects_;
  std::vector<PickId> ids_;
  std::unique_ptr<btCollisionWorld> collision_world_;
};

#endif  // PHYSICS_WORLD_HPP
