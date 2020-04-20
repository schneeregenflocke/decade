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


#include "projection.h"


glm::mat4 Projection::OrthoMatrix(const rect4& viewSize)
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

glm::mat4 Projection::PerspectiveMatrix(float fovy, float near, float far)
{
	return glm::perspective(fovy, AspectRatio(), near, far);
}

float Projection::AspectRatio()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	float width = static_cast<float>(viewport[2]);
	float height = static_cast<float>(viewport[3]);

	return width / height;
}

glm::mat4 Projection::OrthoMatrixWidth(float width)
{
	float x_half_size = width / 2.f;
	float y_half_size = width / AspectRatio() / 2.f;
	float z_half_size = 100.f;

	return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size, -z_half_size, z_half_size);
}

glm::mat4 Projection::OrthoMatrixHeight(float height)
{
	float x_half_size = height * AspectRatio() / 2.f;
	float y_half_size = height / 2.f;
	float z_half_size = 100.f;

	return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size, -z_half_size, z_half_size);
}



View::View()
	: projection_matrix(1.f), view_matrix(1.f), processed_view(1.f), translate_view(0.f), scale_view(1.f)
{
}

void View::SetOrthoMatrix(const rect4& size)
{
	view_size = size;
}

void View::SetViewMatrix(const glm::mat4& matrix)
{
	view_matrix = matrix;
}

glm::mat4& View::GetProjectionMatrix()
{
	projection_matrix = Projection::OrthoMatrix(view_size);
	return projection_matrix;
}

glm::mat4& View::GetViewMatrix()
{
	glm::mat4 identity(1.f);
	processed_view = glm::translate(identity, translate_view);
	processed_view = glm::scale(processed_view, scale_view);
	return processed_view;
}

void View::Translate(const glm::vec3& translate)
{
	translate_view = translate;
}

void View::Scale(const glm::vec3& scale)
{
	scale_view = scale;
}
