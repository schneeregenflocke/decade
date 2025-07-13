/*
Decade
Copyright (c) 2019-2022 Marco Peyer

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

#include "mvp_matrices.hpp"
#include "scene_graph.hpp"
#include "shaders.hpp"
#include <memory>
#include <optional>
#include <string>

class GraphicsEngine {
public:
  void Render()
  {
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (size_t index = 0; index < shaders.GetNumberShaders(); ++index) {
      auto &shader = *shaders.GetShader(index);

      shader.UseProgram();
      shader.SetUniform("projection", mvp.GetProjection());
      shader.SetUniform("view", mvp.GetView());
      shader.SetUniform("model", glm::mat4(1.F));
    }

    scene_graph->draw();
  }

  void SetMVP(const MVP &mvp) { this->mvp = mvp; }

  void set_scene_graph(const std::shared_ptr<SceneNode> &scene_graph)
  {
    this->scene_graph = scene_graph;
  }

  Shaders &GetShaders() { return shaders; }

  std::optional<Shader *> search_shader(const std::string &search_name)
  {
    return shaders.search_shader(search_name);
  }

private:
  MVP mvp;
  Shaders shaders;
  std::shared_ptr<SceneNode> scene_graph;
};
