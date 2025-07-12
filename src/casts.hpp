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

// #include "wx_widgets_include.hpp"

#include <glm/vec4.hpp>

#include <array>
#include <wx/colour.h>

inline glm::vec4 to_glm_vec4(const wxColour &color)
{
  const float ratio = 1.f / 255.f;

  float red = static_cast<float>(color.Red()) * ratio;
  float green = static_cast<float>(color.Green()) * ratio;
  float blue = static_cast<float>(color.Blue()) * ratio;
  float alpha = static_cast<float>(color.Alpha()) * ratio;

  return glm::vec4(red, green, blue, alpha);
}

inline glm::vec4 to_glm_vec4(const std::array<float, 4> values)
{
  return glm::vec4(values[0], values[1], values[2], values[3]);
}

inline wxColour to_wx_color(const glm::vec4 &color)
{
  unsigned char red = static_cast<unsigned char>(color.r * 255.f);
  unsigned char green = static_cast<unsigned char>(color.g * 255.f);
  unsigned char blue = static_cast<unsigned char>(color.b * 255.f);
  unsigned char alpha = static_cast<unsigned char>(color.a * 255.f);

  return wxColour(red, green, blue, alpha);
}
