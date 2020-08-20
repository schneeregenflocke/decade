/*
Decade
Copyright (c) 2019-2020 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

#pragma once

#include <glm/vec4.hpp>

#include <wx/colour.h>

#include <string>

#include <random>


class RectangleShapeConfig
{
public:

	RectangleShapeConfig();
	RectangleShapeConfig(const std::wstring& name);
	RectangleShapeConfig(const std::wstring& name, bool outline_visible, bool fill_visible, float linewidth, const glm::vec4& outline_color, const glm::vec4& fill_color);

	std::wstring Name() const;

	void FillVisible(bool value);
	void OutlineVisible(bool value);

	bool FillVisible() const;
	bool OutlineVisible() const;

	void LineWidth(float value);
	
	void OutlineColor(glm::vec4 value);
	void FillColor(glm::vec4 value);

	float LineWidth() const;

	glm::vec4 OutlineColor() const;
	glm::vec4 FillColor() const;

	float LineWidthDisabled() const;

	glm::vec4 OutlineColorDisabled() const;
	glm::vec4 FillColorDisabled() const;

	void RandomColor(float dim_outline_color_factor);
	

	bool operator==(const RectangleShapeConfig& compare_object);

	static std::vector<RectangleShapeConfig>::iterator GetShapeConfig(const std::wstring& name, std::vector<RectangleShapeConfig>* configs);

private:

	std::wstring name;
	bool outline_visible;
	bool fill_visible;
	float linewidth;
	glm::vec4 outline_color;
	glm::vec4 fill_color;
};


glm::vec4 to_glm_color(const wxColour& color);
wxColour to_wx_color(const glm::vec4& color);
