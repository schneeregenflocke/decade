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

#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using mat4 = glm::mat4;
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Resource.h"



struct VertexVC
{
	vec3 point;
	vec4 color;

	static void SetAttribPointers();
	static void EnableVertexAttribArrays();
	static void DisableVertexAttribArrays();
};


struct VertexVT
{
	glm::vec3 position;
	glm::vec2 texturePosition;

	static void SetAttribPointers();
	static void EnableVertexAttribArrays();
	static void DisableVertexAttribArrays();
};




class Shader
{
public:

	void UseProgram();
	GLuint GetProgram();

protected:
	void CompileProgram(Resource vertexshaderfile, Resource fragmentshaderfile);
	static GLuint LinkShaders(const GLuint vertexshader, const GLuint fragmentshader);
	static GLuint CompileShader(const Resource& resource, const GLuint shadertype);

private:
	GLuint program;
};


class ProjectionShader
{
public:
	void GetProjectionLocations(GLuint program);

	GLint model;
	GLint view;
	GLint projection;

	void SetModelMatrix(const mat4& matrix) const;
	void SetViewMatrix(const mat4& matrix) const;
	void SetProjectionMatrix(const mat4& matrix) const;
};


class LightShader
{
public:
	void GetLightLocations(GLuint program);
	void SetLightPosition(const vec3& position) const;

	GLint lightposition;
};


class SimpleShader : public Shader, public ProjectionShader
{
public:
	using VertexType = VertexVC;
	SimpleShader();
};


class PhongShader : public Shader, public ProjectionShader, public LightShader
{
public:
	PhongShader();
};


class FontShader : public Shader, public ProjectionShader
{
public:
	using VertexType = VertexVT;
	FontShader();
	GLint textColorLocation;
};


#endif /* SHADERS_H */
