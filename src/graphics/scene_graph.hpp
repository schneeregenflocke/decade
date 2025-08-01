/*
Dekade
Copyright (c) 2019-2024 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "shapes.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
  SceneNode() : model_matrix(1.f), is_dirty(false) {}

  SceneNode(const std::string &node_name) : SceneNode() { this->node_name = node_name; }

  SceneNode(const std::string &node_name, std::shared_ptr<Shape> shape) : SceneNode(node_name)
  {
    this->shape = shape;
  }

  void add_child(std::shared_ptr<SceneNode> child) { children.push_back(child); }

  auto add_child()
  {
    std::shared_ptr<SceneNode> child;
    children.push_back(child);
    return child;
  }

  auto get_children() { return children; }

  void remove_children() { children.clear(); }

  void set_shape(std::shared_ptr<Shape> shape) { this->shape = shape; }

  std::shared_ptr<Shape> get_shape() { return shape; }

  void update()
  {
    for (auto &&child : children) {
      child->update();
    }
  }

  std::shared_ptr<SceneNode> search_node(const std::string &search_name)
  {
    if (this->node_name == search_name) {
      return std::shared_ptr<SceneNode>(this->shared_from_this());
    }

    for (auto &&child : children) {
      auto result = child->search_node(search_name);
      if (result != nullptr) {
        return result;
      }
    }

    return nullptr;
  }

  void draw()
  {
    if (shape != nullptr) {
      shape->draw();
    }

    for (auto &&child : children) {
      child->draw();
    }
  }

private:
  std::string node_name;
  std::vector<std::shared_ptr<SceneNode>> children;
  glm::mat4 model_matrix;
  std::shared_ptr<Shape> shape;
  bool is_dirty;
};
