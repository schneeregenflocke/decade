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

#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_TEXTURE_OBJECT_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_TEXTURE_OBJECT_HPP

#include <epoxy/gl.h>

#include <utility>

class Texture {
public:
  Texture() { glGenTextures(1, &name); }

  ~Texture()
  {
    if (name != 0) {
      glDeleteTextures(1, &name);
    }
  }

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  Texture(Texture &&other) noexcept : name(std::exchange(other.name, 0)) {}
  Texture &operator=(Texture &&other) noexcept
  {
    if (this != &other) {
      if (name != 0) {
        glDeleteTextures(1, &name);
      }
      name = std::exchange(other.name, 0);
    }
    return *this;
  }

  [[nodiscard]] GLuint Name() const { return name; }

private:
  GLuint name{0};
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_TEXTURE_OBJECT_HPP
