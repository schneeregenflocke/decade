#include "child_pool.hpp"

#include <memory>
#include <string>
#include <utility>

#include "scene_graph.hpp"

ChildPool::ChildPool(std::shared_ptr<SceneNode> parent)
    : parent_(std::move(parent)) {}

ChildPool::~ChildPool() { parent_->TruncateChildren(used_); }

std::shared_ptr<SceneNode> ChildPool::Next(const std::string& name) {
  if (used_ == parent_->GetChildren().size()) {
    parent_->AddChild(std::make_shared<SceneNode>(name));
  }
  std::shared_ptr<SceneNode> child = parent_->GetChildren()[used_];
  ++used_;
  child->SetNodeName(name);
  return child;
}
