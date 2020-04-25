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

#ifndef SHAPES_H
#define SHAPES_H


#include "rect4.h"
#include "shaders.h"
#include "shapes_base.h"

//#define GLM_FORCE_MESSAGES
#include <glm/vec3.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <glm/gtc/constants.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;

#include <vector>
#include <array>
#include <cmath>



class TriangleShape : public Shape<SimpleShader>
{
public:
	TriangleShape();
	void SetShape(const vec3& p0, const vec3& p1, const vec3& p2);
};


class CircleBase : public Shape<SimpleShader>
{
public:
	CircleBase();
	void SetResolution(size_t value);
protected:
	static vec3 CirclePoint(float angle, float radius);
	size_t resolution;
};


class CircleShape : public CircleBase
{
public:
	void SetShape(const vec3& position, float radius, vec4 color);
};


class CircleSectorShape : public CircleBase
{
public:
	void SetShape(const vec3& position, float radius, float beginsector, float endsector);
};


class QuadShape : public Shape<SimpleShader>
{
public:
	QuadShape();
	void SetShape(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, const vec4& color);
	void SetShape(const rect4& rectangle, const vec4& color);
};


class OrthoLineShape : public Shape<SimpleShader>
{
public:
	OrthoLineShape();
	void SetShape(const vec3& p0, const vec3& p1, float width);
};


class LineShape : public Shape<SimpleShader>
{
public:
	LineShape();
	void SetShape(const vec3& p0, const vec3& p1, float width);
};


class CuboidShape : public Shape<SimpleShader>
{
public:
	CuboidShape();
	void SetShape(const vec3& center, float width, float height, float depth);
};


class RectanglesShape : public Shape<SimpleShader>
{
public:
	void SetShapes(const std::vector<rect4>& rectangles, const std::vector<float>& linewidths, const std::vector<vec4>& fillcolors, const std::vector<vec4>& outlinecolors);
	void SetShapes(const std::vector<rect4>& rectangles, float outlinethickness, const std::vector<vec4>& fillcolors, const vec4& outlinecolor);
	void SetShapes(const std::vector<rect4>& rectangles, float outlinethickness, const vec4& fillcolor, const vec4& outlinecolor);
	void SetShapes(const rect4 rectangle, float outlinethickness, const vec4& fillcolor, const vec4& outlinecolor);

private:
	void SetSlice(size_t index, const rect4& rectangle, float outlinethickness, const vec4& fillcolor, const vec4& outlinecolor);
	void SetSubSlice(size_t first, const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, const vec4& color);
};


#endif /*SHAPES_H*/
