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

#include "shapes.h"


TriangleShape::TriangleShape()
{
	SetBufferSize(3);
}

void TriangleShape::SetShape(const vec3& p0, const vec3& p1, const vec3& p2)
{
	GetVertexRef(0).point = p0;
	GetVertexRef(1).point = p1;
	GetVertexRef(2).point = p2;

	UpdateBuffer();
}


CircleBase::CircleBase()
{
	SetResolution(64);
}

vec3 CircleBase::CirclePoint(float angle, float radius)
{
	return vec3(glm::cos(angle) * radius, glm::sin(angle) * radius, 0.f);
}

void CircleBase::SetResolution(size_t value)
{
	SetBufferSize(value * 3);
	resolution = value;
}


void CircleShape::SetShape(const vec3& position, float radius, vec4 color)
{
	float step = glm::two_pi<float>() / static_cast<float>(resolution);

	auto p0 = vec3(0.f, 0.f, 0.f);
	auto p1 = CirclePoint(0.f, radius);
	auto p2 = CirclePoint(step, radius);

	for (size_t index = 0; index < resolution; ++index)
	{
		auto angle = step * static_cast<float>(index);

		auto rp0 = glm::rotateZ(p0, angle);
		auto rp1 = glm::rotateZ(p1, angle);
		auto rp2 = glm::rotateZ(p2, angle);

		size_t subindex = index * 3;
		
		GetVertexRef(subindex + 0).point = rp0 + position;
		GetVertexRef(subindex + 1).point = rp1 + position;
		GetVertexRef(subindex + 2).point = rp2 + position;

		GetVertexRef(subindex + 0).color = color;
		GetVertexRef(subindex + 1).color = color;
		GetVertexRef(subindex + 2).color = color;
	}

	UpdateBuffer();
}


void CircleSectorShape::SetShape(const vec3& position, float radius, float beginsector, float endsector)
{
	float step = (beginsector - endsector) / static_cast<float>(resolution);

	auto p0 = vec3(0.f, 0.f, 0.f);
	auto p1 = CirclePoint(beginsector, radius);
	auto p2 = CirclePoint(beginsector + step, radius);

	for (size_t index = 0; index < resolution; ++index)
	{
		auto angle = step * static_cast<float>(index);

		auto rp0 = glm::rotateZ(p0, angle);
		auto rp1 = glm::rotateZ(p1, angle);
		auto rp2 = glm::rotateZ(p2, angle);

		auto subindex = index * 3U;

		GetVertexRef(subindex + 0).point = rp0 + position;
		GetVertexRef(subindex + 1).point = rp1 + position;
		GetVertexRef(subindex + 2).point = rp2 + position;
	}

	UpdateBuffer();
}


QuadShape::QuadShape()
{
	SetBufferSize(6);
}

void QuadShape::SetShape(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, const vec4& color)
{
	GetVertexRef(0).point = p0;
	GetVertexRef(1).point = p1;
	GetVertexRef(2).point = p2;
	GetVertexRef(3).point = p3;
	GetVertexRef(4).point = p2;
	GetVertexRef(5).point = p1;

	GetVertexRef(0).color = color;
	GetVertexRef(1).color = color;
	GetVertexRef(2).color = color;
	GetVertexRef(3).color = color;
	GetVertexRef(4).color = color;
	GetVertexRef(5).color = color;

	UpdateBuffer();
}


void QuadShape::SetShape(const rect4& rectangle, const vec4& color)
{
	GetVertexRef(0).point = vec3(rectangle.Left(), rectangle.Bottom(), 0.f);
	GetVertexRef(1).point = vec3(rectangle.Right(), rectangle.Bottom(), 0.f);
	GetVertexRef(2).point = vec3(rectangle.Left(), rectangle.Top(), 0.f);
	GetVertexRef(3).point = vec3(rectangle.Right(), rectangle.Top(), 0.f);
	GetVertexRef(4).point = vec3(rectangle.Left(), rectangle.Top(), 0.f);
	GetVertexRef(5).point = vec3(rectangle.Right(), rectangle.Bottom(), 0.f);

	GetVertexRef(0).color = color;
	GetVertexRef(1).color = color;
	GetVertexRef(2).color = color;
	GetVertexRef(3).color = color;
	GetVertexRef(4).color = color;
	GetVertexRef(5).color = color;

	UpdateBuffer();
}



LineShape::LineShape()
{
	SetBufferSize(6);
}

void LineShape::SetShape(const vec3& p0, const vec3& p1, float width)
{
	auto direction = p1 - p0;
	auto normdirection = glm::normalize(direction);
	auto halfwidth = width / 2.f;
	auto scalednormdirection = normdirection * halfwidth;

	vec3 hvec0(scalednormdirection.y, -scalednormdirection.x, 0.f); // CW
	vec3 hvec1(-scalednormdirection.y, scalednormdirection.x, 0.f); // CCW

	auto qp0 = p0 + hvec0;
	auto qp1 = p1 + hvec0;
	auto qp2 = p0 + hvec1;
	auto qp3 = p1 + hvec1;

	GetVertexRef(0).point = qp0;
	GetVertexRef(1).point = qp1;
	GetVertexRef(2).point = qp2;
	GetVertexRef(3).point = qp3;
	GetVertexRef(4).point = qp2;
	GetVertexRef(5).point = qp1;

	UpdateBuffer();
}


OrthoLineShape::OrthoLineShape()
{
	SetBufferSize(6);
}

void OrthoLineShape::SetShape(const vec3& p0, const vec3& p1, float width)
{
	auto halfwidth = width / 2.f;

	vec3 hvec0(-halfwidth, -halfwidth, 0.f);
	vec3 hvec1( halfwidth, -halfwidth, 0.f);
	vec3 hvec2(-halfwidth,  halfwidth, 0.f);
	vec3 hvec3( halfwidth,  halfwidth, 0.f);

	auto qp0 = p0 + hvec0;
	auto qp1 = p1 + hvec1;
	auto qp2 = p0 + hvec2;
	auto qp3 = p1 + hvec3;

	GetVertexRef(0).point = qp0;
	GetVertexRef(1).point = qp1;
	GetVertexRef(2).point = qp2;
	GetVertexRef(3).point = qp1;
	GetVertexRef(4).point = qp3;
	GetVertexRef(5).point = qp2;

	UpdateBuffer();
}


CuboidShape::CuboidShape()
{
	SetBufferSize(36);
}

void CuboidShape::SetShape(const vec3& center, float width, float height, float depth)
{
	float halfwidth = width * 0.5f;
	float halfheight = height * 0.5f;
	float halfdepth = depth * 0.5f;

	std::array<vec3, 8> points;

	points[0] = center + vec3(-halfwidth, -halfheight,  halfdepth);
	points[1] = center + vec3( halfwidth, -halfheight,  halfdepth);
	points[2] = center + vec3(-halfwidth,  halfheight,  halfdepth);
	points[3] = center + vec3( halfwidth,  halfheight,  halfdepth);

	points[4] = center + vec3( halfwidth, -halfheight, -halfdepth);
	points[5] = center + vec3(-halfwidth, -halfheight, -halfdepth);
	points[6] = center + vec3( halfwidth,  halfheight, -halfdepth);
	points[7] = center + vec3(-halfwidth,  halfheight, -halfdepth);

	std::array<size_t, 36> indices = { 
		0, 1, 2, 3, 2, 1, // front quad
		4, 5, 6, 7, 6, 5, // back quad
		1, 4, 3, 6, 3, 4, // right quad
		5, 0, 7, 2, 7, 0, // left quad
		2, 3, 7, 6, 7, 3, // top quad
		5, 4, 0, 1, 0, 4  // bottom quad
	};

	for (auto index = 0U; index < 36; ++index)
	{
		GetVertexRef(index).point = points[indices[index]];
	}

	UpdateBuffer();
}




void RectanglesShape::SetShapes(const std::vector<rect4>& rectangles, float outlinethickness, const std::vector<vec4>& fillcolors, const vec4& outlinecolor)
{
	size_t size = 6 * 5 * rectangles.size();
	SetBufferSize(size);

	for (size_t index = 0; index < rectangles.size(); ++index)
	{
		SetSlice(index, rectangles[index], outlinethickness, fillcolors[index], outlinecolor);
	}

	UpdateBuffer();
}

void RectanglesShape::SetShapes(const std::vector<rect4>& rectangles, float outlinethickness, const vec4& fillcolor, const vec4& outlinecolor)
{
	size_t size = 6 * 5 * rectangles.size();
	SetBufferSize(size);

	for (size_t index = 0; index < rectangles.size(); ++index)
	{
		SetSlice(index, rectangles[index], outlinethickness, fillcolor, outlinecolor);
	}

	UpdateBuffer();
}

void RectanglesShape::SetShapes(const rect4 rectangle, float outlinethickness, const vec4& fillcolor, const vec4& outlinecolor)
{
	auto size = 6 * 5;
	SetBufferSize(size);

	SetSlice(0, rectangle, outlinethickness, fillcolor, outlinecolor);
	
	UpdateBuffer();
}


void RectanglesShape::SetSlice(size_t index, const rect4& rectangle, float outlinethickness, const vec4& fillcolor, const vec4& outlinecolor)
{
	float hlt = outlinethickness / 2.f;

	rect4 inrectangle = rectangle.Reduce(rect4(hlt, hlt, hlt, hlt));
	rect4 outrectangle = rectangle.Expand(rect4(hlt, hlt, hlt, hlt));
	
	SetSubSlice(index * 6 * 5 + 0 * 6, inrectangle.GetLB(), inrectangle.GetRB(), inrectangle.GetLT(), inrectangle.GetRT(), fillcolor);
	SetSubSlice(index * 6 * 5 + 1 * 6, inrectangle.GetLT(), inrectangle.GetRT(), outrectangle.GetLT(), outrectangle.GetRT(), outlinecolor);
	SetSubSlice(index * 6 * 5 + 2 * 6, outrectangle.GetLB(), outrectangle.GetRB(), inrectangle.GetLB(), inrectangle.GetRB(), outlinecolor);
	SetSubSlice(index * 6 * 5 + 3 * 6, outrectangle.GetLB(), inrectangle.GetLB(), outrectangle.GetLT(), inrectangle.GetLT(), outlinecolor);
	SetSubSlice(index * 6 * 5 + 4 * 6, inrectangle.GetRB(), outrectangle.GetRB(), inrectangle.GetRT(), outrectangle.GetRT(), outlinecolor);
}


void RectanglesShape::SetSubSlice(size_t first, const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, const vec4& color)
{
	GetVertexRef(first + 0).point = p0;
	GetVertexRef(first + 1).point = p1;
	GetVertexRef(first + 2).point = p2;
	GetVertexRef(first + 3).point = p3;
	GetVertexRef(first + 4).point = p2;
	GetVertexRef(first + 5).point = p1;

	GetVertexRef(first + 0).color = color;
	GetVertexRef(first + 1).color = color;
	GetVertexRef(first + 2).color = color;
	GetVertexRef(first + 3).color = color;
	GetVertexRef(first + 4).color = color;
	GetVertexRef(first + 5).color = color;
}
