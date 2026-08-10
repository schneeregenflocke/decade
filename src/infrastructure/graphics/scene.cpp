#include "scene.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <memory>

#include "scene_graph.hpp"

Scene::Scene() : root_(std::make_unique<SceneNode>(kRootName)) {}

SceneNode& Scene::Root() { return *root_; }

const SceneNode& Scene::Root() const { return *root_; }

void Scene::Draw(const glm::mat4& parent_world) { root_->Draw(parent_world); }
