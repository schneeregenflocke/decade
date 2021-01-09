/*
Decade
Copyright (c) 2019-2021 Marco Peyer

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.
*/

#pragma once

//#define GLM_FORCE_MESSAGES
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>



class rect4
{
public:
	rect4() :
		left(0.f),
		bottom(0.f),
		right(0.f),
		top(0.f)
	{}

	rect4(float left, float bottom, float right, float top) :
		left(left), 
		bottom(bottom), 
		right(right), 
		top(top)
	{}
	
	rect4(float width, float height)
	{
		left = -width / 2.0f;
		right = width / 2.0f;
		bottom = -height / 2.0f;
		top = height / 2.0f;
	}

	float Left() const
	{
		return left;
	}

	float Bottom() const
	{
		return bottom;
	}

	float Right() const
	{
		return right;
	}

	float Top() const
	{
		return top;
	}

	void Left(float value)
	{
		left = value;
	}

	void Bottom(float value)
	{
		bottom = value;
	}

	void Right(float value)
	{
		right = value;
	}

	void Top(float value)
	{
		top = value;
	}

	float Width() const
	{
		return right - left;
	}

	float Height() const
	{
		return top - bottom;
	}
	
	rect4 Expand(const rect4& expand) const
	{
		rect4 expanded;
		expanded.Left(left - expand.left);
		expanded.Bottom(bottom - expand.bottom);
		expanded.Right(right + expand.right);
		expanded.Top(top + expand.top);
		return expanded;
	}

	rect4 Reduce(const rect4& reduce) const
	{
		rect4 reduced;
		reduced.Left(left + reduce.left);
		reduced.Bottom(bottom + reduce.bottom);
		reduced.Right(right - reduce.right);
		reduced.Top(top - reduce.top);
		return reduced;
	}

	rect4 Scale(float factor) const
	{
		float halfWidthDiff = ((Width() * factor) - Width()) / 2.f;
		float halfHeightDiff = ((Height() * factor) - Height()) / 2.f;
		rect4 scaled = Expand(rect4(halfWidthDiff, halfWidthDiff, halfHeightDiff, halfHeightDiff));
		return scaled;
	}

	glm::vec3 Center() const
	{
		return glm::vec3(left + Width() / 2.f, bottom + Height() / 2.f, 0.f);
	}
	
	glm::vec3 GetLB() const
	{
		//return vec3(left, bottom, 0.f);
		return glm::vec3(glm::vec2(left, bottom), 0.f);
	}
	glm::vec3 GetRB() const
	{
		return glm::vec3(right, bottom, 0.f);
	}
	glm::vec3 GetLT() const
	{
		return glm::vec3(left, top, 0.f);
	}
	glm::vec3 GetRT() const
	{
		return glm::vec3(right, top, 0.f);
	}

private:
	float left;
	float bottom;
	float right;
	float top;
};
