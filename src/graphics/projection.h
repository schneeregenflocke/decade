/*
Decade
Copyright (c) 2019-2020 Marco Peyer

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


#include "rect4.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>


class Projection
{
public:
	static float AspectRatio()
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		float width = static_cast<float>(viewport[2]);
		float height = static_cast<float>(viewport[3]);

		return width / height;
	}

	static glm::mat4 OrthoMatrix(const rect4& viewSize)
	{
		float page_height_ratio = viewSize.Width() / viewSize.Height();
		float viewport_height_ratio = AspectRatio();

		glm::mat4 ortho_matrix;
		if (page_height_ratio >= viewport_height_ratio)
		{
			ortho_matrix = OrthoMatrixWidth(viewSize.Width());
		}
		else
		{
			ortho_matrix = OrthoMatrixHeight(viewSize.Height());
		}
		return ortho_matrix;
	}

	static glm::mat4 PerspectiveMatrix(const float fovy, const float z_near, const float z_far)
	{
		return glm::perspective(fovy, AspectRatio(), z_near, z_far);
	}

	static glm::mat4 OrthoMatrixWidth(float width)
	{
		float x_half_size = width / 2.f;
		float y_half_size = width / AspectRatio() / 2.f;
		//float z_half_size = 100.f;

		//return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size, -z_half_size, z_half_size);
		return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
	}

	static glm::mat4 OrthoMatrixHeight(float height)
	{
		float x_half_size = height * AspectRatio() / 2.f;
		float y_half_size = height / 2.f;
		//float z_half_size = 100.f;

		//return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size, -z_half_size, z_half_size);
		return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
	}
};


class View
{
public:

	View() : 
		projection_matrix(1.f),
		view_matrix(1.f)
	{}

	void SetMinRect(const rect4& min_rect)
	{
		this->min_rect = min_rect;
	}

	void UpdateOrthoMatrix()
	{
		projection_matrix = Projection::OrthoMatrix(min_rect);
	}

	void SetViewMatrix(const glm::mat4& view_matrix)
	{
		this->view_matrix = view_matrix;
	}

	glm::mat4& GetProjectionMatrix()
	{
		UpdateOrthoMatrix();
		return projection_matrix;
	}

	glm::mat4& GetViewMatrix()
	{
		return view_matrix;
	}

private:
	glm::mat4 projection_matrix;
	glm::mat4 view_matrix;

	rect4 min_rect;
};
