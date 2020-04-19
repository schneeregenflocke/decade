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

//#define GLM_FORCE_MESSAGES
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

//typedef glm::vec3 vec3;
//typedef glm::vec3 vec2;




class rect4
{
public:
	rect4();
	rect4(float left, float bottom, float right, float top);
	
	rect4(float width, float height);

	float Left() const;
	float Bottom() const;
	float Right() const;
	float Top() const;

	void Left(float value);
	void Bottom(float value);
	void Right(float value);
	void Top(float value);

	float Width() const;
	float Height() const;
	
	rect4 Expand(const rect4& expand) const;
	rect4 Reduce(const rect4& reduce) const;
	rect4 Scale(float factor) const;

	glm::vec3 Center() const;
	
	glm::vec3 GetLB() const;
	glm::vec3 GetRB() const;
	glm::vec3 GetLT() const;
	glm::vec3 GetRT() const;

private:
	float left;
	float bottom;
	float right;
	float top;
};




