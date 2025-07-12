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

#include "rect.hpp"

#include <glad/glad.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// #include <iostream>

class Projection {
public:
  static float AspectRatio()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    float width = static_cast<float>(viewport[2]);
    float height = static_cast<float>(viewport[3]);

    return width / height;
  }

  static glm::mat4 OrthoMatrix(const rectf &view_size)
  {
    float page_height_ratio = view_size.width() / view_size.height();
    float viewport_height_ratio = AspectRatio();

    glm::mat4 ortho_matrix;
    if (page_height_ratio >= viewport_height_ratio) {
      ortho_matrix = OrthoMatrixWidth(view_size.width());
    } else {
      ortho_matrix = OrthoMatrixHeight(view_size.height());
    }

    return ortho_matrix;
  }

  static glm::mat4 PerspectiveMatrix(const float fovy, const float z_near, const float z_far)
  {
    return glm::perspective(fovy, AspectRatio(), z_near, z_far);
  }

  static glm::mat4 OrthoMatrixWidth(float width)
  {
    float x_half_size = width / 2.f;
    float y_half_size = width / AspectRatio() / 2.f;
    return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
  }

  static glm::mat4 OrthoMatrixHeight(float height)
  {
    float x_half_size = height * AspectRatio() / 2.f;
    float y_half_size = height / 2.f;
    return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
  }
};
