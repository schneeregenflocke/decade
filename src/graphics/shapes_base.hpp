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


#include "shaders.hpp"

#include <vector>
#include <iostream>

//#define GLM_FORCE_MESSAGES
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

#include <glad/glad.h>



class VertexArrayObject
{
public:
	explicit VertexArrayObject()
	{
		glGenVertexArrays(1, &VAO);
	}
	virtual ~VertexArrayObject()
	{
		glDeleteVertexArrays(1, &VAO);
	}
protected:
	GLuint VAO;
};


class VertexBufferObject
{
public:
	explicit VertexBufferObject()
	{
		glGenBuffers(1, &VBO);
	}
	virtual ~VertexBufferObject()
	{
		glDeleteBuffers(1, &VBO);
	}
protected:
	GLuint VBO;
};


class ShapeBase : public VertexArrayObject, public VertexBufferObject
{
public:

	explicit ShapeBase(std::string shape_name) :
		vertices_size(0),
		shader_ptr(nullptr),
		shape_name(shape_name)
	{}

	virtual void Draw() const
	{
		shader_ptr->UseProgram();
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_size));
	}

	void SetShader(Shader* shader_ptr)
	{
		this->shader_ptr = shader_ptr;
	}

	std::string& GetShapeName()
	{
		return shape_name;
	}

protected:

	size_t vertices_size;
	Shader* shader_ptr;
	std::string shape_name;
};


template<typename T>
class Shape : public ShapeBase
{
public:

	using ShaderType = T;
	using U = typename T::VertexType;

	explicit Shape(const std::string shape_name) :
		ShapeBase(shape_name),
		buffer_size(0)
	{
		InitBuffer();
	}

protected:

	void InitBuffer()
	{
		glBindVertexArray(VAO);
		U::EnableVertexAttribArrays();

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		U::SetAttribPointers();
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void SetBufferSize(GLsizeiptr vertices_size)
	{
		this->vertices_size = vertices_size;
		buffer_size = vertices_size * sizeof(U);

		vertices.resize(vertices_size);
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void UpdateBuffer()
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	U& GetVertexRef(size_t index)
	{
		return vertices[index];
	}

	size_t GetVerticesSize() const
	{
		return vertices.size();
	}
	
private:
	GLsizeiptr buffer_size;
	std::vector<U> vertices;
};

/*void Shape::CalcNormals()
{
	for (auto index = 0U; index < vertices.size(); index += 3)
	{
		vec3 subvector0 = vertices[index].point - vertices[index + 1].point;
		vec3 subvector1 = vertices[index].point - vertices[index + 2].point;

		vec3 cross = glm::cross(subvector0, subvector1);
		vec3 normal = glm::normalize(cross);

		vertices[index + 0].normal = normal;
		vertices[index + 1].normal = normal;
		vertices[index + 2].normal = normal;
	}
}*/
