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
		glGenVertexArrays(1, &vertex_array_object);
	}
	
	virtual ~VertexArrayObject()
	{
		glDeleteVertexArrays(1, &vertex_array_object);
	}

	void BindVertexArray() const
	{
		glBindVertexArray(vertex_array_object);
	}

	void UnbindVertexArray() const
	{
		glBindVertexArray(0);
	}

	GLuint GetVertexArrayObject() const
	{
		return vertex_array_object;
	}

private:

	GLuint vertex_array_object;
};


class VertexBufferObject
{
public:
	explicit VertexBufferObject()
	{
		glGenBuffers(1, &vertex_buffer_object);
	}

	virtual ~VertexBufferObject()
	{
		glDeleteBuffers(1, &vertex_buffer_object);
	}

	void BindVertexBuffer() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
	}

	void UnbindVertexBuffer() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	GLuint GetVertexBufferObject() const
	{
		return vertex_buffer_object;
	}

private:

	GLuint vertex_buffer_object;
};


class ShapeBase
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
		vertex_array_object.BindVertexArray();
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

	VertexArrayObject vertex_array_object;
	VertexBufferObject vertex_buffer_object;

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
		vertex_array_object.BindVertexArray();
		U::EnableVertexAttribArrays();

		vertex_buffer_object.BindVertexBuffer();
		U::SetAttribPointers(vertex_buffer_object.GetVertexBufferObject());
		
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

		vertex_array_object.UnbindVertexArray();
		vertex_buffer_object.UnbindVertexBuffer();
	}

	void SetBufferSize(GLsizeiptr vertices_size)
	{
		this->vertices_size = vertices_size;
		buffer_size = vertices_size * sizeof(U);

		vertices.resize(vertices_size);
		
		vertex_buffer_object.BindVertexBuffer();
		glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
		vertex_buffer_object.UnbindVertexBuffer();
	}

	void UpdateBuffer()
	{
		vertex_buffer_object.BindVertexBuffer();
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, vertices.data());
		vertex_buffer_object.UnbindVertexBuffer();
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
