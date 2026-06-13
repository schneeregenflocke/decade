#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include <btBulletCollisionCommon.h>

#include <cstddef>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
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
  PhysicsWorld()
      : collision_configuration_(
            std::make_unique<btDefaultCollisionConfiguration>()),
        dispatcher_(std::make_unique<btCollisionDispatcher>(
            collision_configuration_.get())),
        broadphase_(std::make_unique<btDbvtBroadphase>()),
        collision_world_(std::make_unique<btCollisionWorld>(
            dispatcher_.get(), broadphase_.get(),
            collision_configuration_.get())) {}

  // Replaces all registered pickables with the given boxes.
  void Rebuild(const std::vector<PickBox>& boxes) {
    for (const auto& object : objects_) {
      collision_world_->removeCollisionObject(object.get());
    }
    objects_.clear();
    shapes_.clear();
    ids_.clear();

    objects_.reserve(boxes.size());
    shapes_.reserve(boxes.size());
    ids_.reserve(boxes.size());

    for (const auto& box : boxes) {
      const float half_width = box.rect.width() * kHalf;
      const float half_height = box.rect.height() * kHalf;
      const glm::vec3 center = box.rect.getCenter();

      auto shape = std::make_unique<btBoxShape>(
          btVector3(half_width, half_height, kBoxHalfDepth));
      auto object = std::make_unique<btCollisionObject>();
      object->setCollisionShape(shape.get());

      btTransform transform;
      transform.setIdentity();
      transform.setOrigin(btVector3(center.x, center.y, 0.0F));
      object->setWorldTransform(transform);
      object->setUserIndex(static_cast<int>(ids_.size()));

      collision_world_->addCollisionObject(object.get());

      ids_.push_back(box.id);
      shapes_.push_back(std::move(shape));
      objects_.push_back(std::move(object));
    }
  }

  // Returns the PickId of the element under the given page-space point, if any.
  [[nodiscard]] std::optional<PickId> Raycast(glm::vec2 page_point) const {
    const btVector3 from(page_point.x, page_point.y, kRayHalfLength);
    const btVector3 to(page_point.x, page_point.y, -kRayHalfLength);

    btCollisionWorld::ClosestRayResultCallback callback(from, to);
    collision_world_->rayTest(from, to, callback);

    if (!callback.hasHit() || callback.m_collisionObject == nullptr) {
      return std::nullopt;
    }
    const int index = callback.m_collisionObject->getUserIndex();
    if (index < 0 || static_cast<std::size_t>(index) >= ids_.size()) {
      return std::nullopt;
    }
    return ids_[static_cast<std::size_t>(index)];
  }

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
