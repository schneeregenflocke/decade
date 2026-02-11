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

#ifndef HOME_TITAN99_CODE_DECADE_SRC_CASTS_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_CASTS_HPP

// #include "wx_widgets_include.hpp"

#include <glm/vec4.hpp>

#include <array>
#include <wx/colour.h>

inline glm::vec4 to_glm_vec4(const wxColour &color)
{
  constexpr float kColorScale = 1.0F / 255.0F;

  const auto red = static_cast<float>(color.Red()) * kColorScale;
  const auto green = static_cast<float>(color.Green()) * kColorScale;
  const auto blue = static_cast<float>(color.Blue()) * kColorScale;
  const auto alpha = static_cast<float>(color.Alpha()) * kColorScale;

  return {red, green, blue, alpha};
}

inline glm::vec4 to_glm_vec4(const std::array<float, 4> &values)
{
  return {values[0], values[1], values[2], values[3]};
}

inline wxColour to_wx_color(const glm::vec4 &color)
{
  constexpr float kColorMax = 255.0F;
  const auto red = static_cast<unsigned char>(color[0] * kColorMax);
  const auto green = static_cast<unsigned char>(color[1] * kColorMax);
  const auto blue = static_cast<unsigned char>(color[2] * kColorMax);
  const auto alpha = static_cast<unsigned char>(color[3] * kColorMax);

  return {red, green, blue, alpha};
}
#endif // HOME_TITAN99_CODE_DECADE_SRC_CASTS_HPP
