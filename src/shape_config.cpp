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

#include "shape_config.h"


RectangleShapeConfig::RectangleShapeConfig(const std::wstring& name) :
	name(name),
	outline_visible(true),
	fill_visible(false),
	linewidth(0.1f),
	outline_color(0.f, 0.f, 0.f, 1.f),
	fill_color(1.f, 1.f, 1.f, 1.f)
{}

RectangleShapeConfig::RectangleShapeConfig(const std::wstring& name, bool outline_visible, bool fill_visible, float linewidth, const glm::vec4& outline_color, const glm::vec4& fill_color) :
	name(name),
	outline_visible(outline_visible),
	fill_visible(fill_visible),
	linewidth(linewidth),
	outline_color(outline_color),
	fill_color(fill_color)
{}

std::wstring RectangleShapeConfig::Name() const
{
	return name;
}

void RectangleShapeConfig::OutlineVisible(bool value)
{
	outline_visible = value;
}

bool RectangleShapeConfig::OutlineVisible() const
{
	return outline_visible;
}

void RectangleShapeConfig::FillVisible(bool value)
{
	fill_visible = value;
}

bool RectangleShapeConfig::FillVisible() const
{
	return fill_visible;
}

void RectangleShapeConfig::LineWidth(float value)
{
	linewidth = value;
}

float RectangleShapeConfig::LineWidth() const
{
	float return_value = 0.f;
	if (outline_visible == true)
	{
		return_value = linewidth;
	}
	return return_value;
}

void RectangleShapeConfig::OutlineColor(glm::vec4 value)
{
	outline_color = value;
}

void RectangleShapeConfig::FillColor(glm::vec4 value)
{
	fill_color = value;
}

glm::vec4 RectangleShapeConfig::OutlineColor() const
{
	glm::vec4 return_value = glm::vec4(0.f, 0.f, 0.f, 0.f);
	if (outline_visible == true)
	{
		return_value = outline_color;
	}
	return return_value;
}

glm::vec4 RectangleShapeConfig::FillColor() const
{
	glm::vec4 return_value = glm::vec4(0.f, 0.f, 0.f, 0.f);
	if (fill_visible == true)
	{
		return_value = fill_color;
	}	
	return return_value;
}

float RectangleShapeConfig::LineWidthDisabled() const
{
	return linewidth;
}

glm::vec4 RectangleShapeConfig::OutlineColorDisabled() const
{
	return outline_color;
}

glm::vec4 RectangleShapeConfig::FillColorDisabled() const
{
	return fill_color;
}

bool RectangleShapeConfig::operator==(const RectangleShapeConfig& compare_object)
{
	bool return_value = false;
	if (name == compare_object.Name())
	{
		return_value = true;
	}

	return return_value;
}

std::vector<RectangleShapeConfig>::iterator RectangleShapeConfig::GetShapeConfig(const std::wstring& name, std::vector<RectangleShapeConfig>* configs)
{
	return std::find(configs->begin(), configs->end(), name);
}



glm::vec4 to_glm_color(const wxColour& color)
{
	float ratio = 1.f / 255.f;

	float red = static_cast<float>(color.Red()) * ratio;
	float green = static_cast<float>(color.Green()) * ratio;
	float blue = static_cast<float>(color.Blue()) * ratio;
	float alpha = static_cast<float>(color.Alpha()) * ratio;

	return glm::vec4(red, green, blue, alpha);
}

wxColour to_wx_color(const glm::vec4& color)
{
	unsigned char red = static_cast<unsigned char>(color.r * 255.f);
	unsigned char green = static_cast<unsigned char>(color.g * 255.f);
	unsigned char blue = static_cast<unsigned char>(color.b * 255.f);
	unsigned char alpha = static_cast<unsigned char>(color.a * 255.f);
	
	return wxColour(red, green, blue, alpha);
}


