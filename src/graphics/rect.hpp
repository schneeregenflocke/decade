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


#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>


template<typename Ty>
class rect
{
public:

	rect() :
		edges({ 0, 0, 0, 0 })
	{}
	
	rect(Ty l, Ty r, Ty b, Ty t) :
		edges({ l, r, b, t })
	{}
	
	rect(Ty width, Ty height)
	{
		edges[0] = -width  / static_cast<Ty>(2);
		edges[1] =  width  / static_cast<Ty>(2);
		edges[2] = -height / static_cast<Ty>(2);
		edges[3] =  height / static_cast<Ty>(2);
	}

	Ty l() const
	{
		return edges[0];
	}

	Ty r() const
	{
		return edges[1];
	}

	Ty b() const
	{
		return edges[2];
	}

	Ty t() const
	{
		return edges[3];
	}

	Ty width() const
	{
		return edges[1] - edges[0];
	}

	Ty height() const
	{
		return edges[3] - edges[2];
	}

	rect shift(Ty x, Ty y) const
	{
		return rect(l() + x, r() + x, b() + y, t() + y);
	}
	
	rect expand(const rect& value) const
	{
		return rect(l() - value.l(), r() + value.r(), b() - value.b(), t() + value.t());
	}

	rect reduce(const rect& value) const
	{
		return rect(l() + value.l(), r() - value.r(), b() + value.b(), t() - value.t());
	}

	rect scale(Ty factor) const
	{
		const Ty expand_width_value = ((width() * factor) - width()) / static_cast<Ty>(2);
		const Ty expand_height_value = ((height() * factor) - height()) / static_cast<Ty>(2);

		rect result = expand(rect(expand_width_value, expand_width_value, expand_height_value, expand_height_value));
		return result;
	}

	rect dimension(Ty x, Ty y) const
	{
		return rect(l(), l() + x, b(), b() + y);
	}

	glm::vec3 getCenter() const
	{
		return glm::vec3(edges[0] + width() / static_cast<Ty>(2), edges[2] + height() / static_cast<Ty>(2), static_cast<Ty>(0));
	}
	
	glm::vec3 getLB() const
	{
		return glm::vec3(edges[0], edges[2], static_cast<Ty>(0));
	}

	glm::vec3 getRB() const
	{
		return glm::vec3(edges[1], edges[2], static_cast<Ty>(0));
	}

	glm::vec3 getLT() const
	{
		return glm::vec3(edges[0], edges[3], static_cast<Ty>(0));
	}

	glm::vec3 getRT() const
	{
		return glm::vec3(edges[1], edges[3], static_cast<Ty>(0));
	}

	void setL(Ty value)
	{
		edges[0] = value;
	}

	void setR(Ty value)
	{
		edges[1] = value;
	}

	void setB(Ty value)
	{
		edges[2] = value;
	}

	void setT(Ty value)
	{
		edges[3] = value;
	}
	
private:

	void addL(Ty value)
	{
		edges[0] += value;
	}

	void addR(Ty value)
	{
		edges[1] += value;
	}

	void addB(Ty value)
	{
		edges[2] += value;
	}

	void addT(Ty value)
	{
		edges[3] += value;
	}
	
	std::array<Ty, 4> edges;
};

using rectf = rect<float>;
