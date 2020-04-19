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

#ifndef PROJECTION_H
#define PROJECTION_H


#include "rect4.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_projection.hpp>

//#define GLEW_STATIC
//#include <GL/glew.h>
#include <glad/glad.h>

#include <iostream>


class Projection
{
public:
	static glm::mat4 OrthoMatrix(const rect4& viewSize);
	static glm::mat4 PerspectiveMatrix(float fovy, float near, float far);
	static float AspectRatio();
	static glm::mat4 OrthoMatrixWidth(float width);
	static glm::mat4 OrthoMatrixHeight(float height);
};


class View
{
public:

	View();

	void SetOrthoMatrix(const rect4& size);
	void SetViewMatrix(const glm::mat4& matrix);

	glm::mat4& GetProjectionMatrix();
	glm::mat4& GetViewMatrix();

	void Translate(const glm::vec3& translate);
	void Scale(const glm::vec3& scale);

private:
	glm::mat4 projection_matrix;
	glm::mat4 view_matrix;
	glm::mat4 processed_view;
	
	glm::vec3 translate_view;
	glm::vec3 scale_view;

	rect4 view_size;
};

#endif /*PROJECTION_H*/
