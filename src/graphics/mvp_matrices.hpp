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

#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_MVP_MATRICES_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_MVP_MATRICES_HPP

#include <glm/glm.hpp>

class MVP {
public:
  MVP() : projection(1.0F), view(1.0F), model(1.0F) {}

  void SetProjection(const glm::mat4 &projection) { this->projection = projection; }

  void SetView(const glm::mat4 &view) { this->view = view; }

  [[nodiscard]] glm::mat4 GetProjection() const { return projection; }

  [[nodiscard]] glm::mat4 GetView() const { return view; }

private:
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_MVP_MATRICES_HPP
