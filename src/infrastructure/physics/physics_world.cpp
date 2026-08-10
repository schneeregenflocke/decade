#include "physics_world.hpp"

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btVector3.h>

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "../graphics/pick_id.hpp"

PhysicsWorld::PhysicsWorld()
    : collision_configuration_(
          std::make_unique<btDefaultCollisionConfiguration>()),
      dispatcher_(std::make_unique<btCollisionDispatcher>(
          collision_configuration_.get())),
      broadphase_(std::make_unique<btDbvtBroadphase>()),
      collision_world_(std::make_unique<btCollisionWorld>(
          dispatcher_.get(), broadphase_.get(),
          collision_configuration_.get())) {}

void PhysicsWorld::Rebuild(const std::vector<PickBox>& boxes) {
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
    const float half_width = box.rect.Width() * kHalf;
    const float half_height = box.rect.Height() * kHalf;
    const glm::vec3 center = box.rect.Center();

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

std::optional<PickId> PhysicsWorld::Raycast(glm::vec2 page_point) const {
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
