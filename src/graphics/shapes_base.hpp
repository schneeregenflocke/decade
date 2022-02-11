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


//#define GLEW_STATIC
//#include <GL/glew.h>
#include <glad/glad.h>



class VertexArrayObject
{
public:
	VertexArrayObject()
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
	VertexBufferObject()
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

	virtual void Draw() const
	{
		shader->UseProgram();
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_size));
	}

	void SetShader(Shader* shaderPtr)
	{
		shader = shaderPtr;
	}

protected:

	size_t vertices_size;
	Shader* shader;


private:
};


template<typename T>
class Shape : public ShapeBase
{
public:

	using ShaderType = T;
	using U = typename T::VertexType;

	Shape() : 
		buffersize(0)
	{
		InitializeBuffer();
	}

	/*Shape::Shape()
	{
		buffersize = 0;
		InitializeBuffer();
	}*/
	
	Shape(const Shape& shape) = delete;
	Shape& operator=(const Shape&) = delete;


protected:

	void InitializeBuffer()
	{
		glBindVertexArray(VAO);
		U::EnableVertexAttribArrays();

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		U::SetAttribPointers();
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glBindVertexArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void SetBufferSize(GLsizeiptr verticessize)
	{
		buffersize = verticessize * sizeof(U);
		vertices.resize(verticessize);
		vertices_size = verticessize;

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, buffersize, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void UpdateBuffer()
	{
		//CalcNormals();

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffersize, vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	//vec3& GetPointRef(size_t index);
	//vec4& GetColorRef(size_t index);

	U& GetVertexRef(size_t index)
	{
		return vertices[index];
	}

	size_t GetVerticesSize() const
	{
		return vertices.size();
	}
	
private:
	GLsizeiptr buffersize;
	std::vector<U> vertices;

	//GLint GetBufferSize();
	//void CalcNormals();
};






/*Shape::Shape(const Shape& shape)
{
	InitializeBuffer();
	vertices = shape.vertices;
	SetBufferSize(shape.vertices.size(), GL_DYNAMIC_DRAW);
	UpdateBuffer();
}*/


/*void Shape::InitializeBuffer()
{
	glBindVertexArray(VAO);
	VertexVC::EnableVertexAttribArrays();

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	VertexVC::SetAttribPointers();
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}*/


/*void Shape::SetBufferSize(GLsizeiptr verticessize, GLenum bufferusage = GL_DYNAMIC_DRAW)
{
	buffersize = verticessize * sizeof(VertexVC);
	vertices.resize(verticessize);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, buffersize, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}*/


/*void Shape::UpdateBuffer()
{
	//CalcNormals();

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, buffersize, vertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}*/


/*vec3& Shape::GetPointRef(size_t index)
{
	return vertices[index].point;
}


vec4& Shape::GetColorRef(size_t index)
{
	return vertices[index].color;
}*/

/*VertexVC& Shape::GetVertexRef(size_t index)
{
	return vertices[index];
}*/


/*size_t Shape::GetVerticesSize() const
{
	return vertices.size();
}*/


/*void Shape::Draw() const
{
	glBindVertexArray(VAO);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}*/

/*GLint Shape::GetBufferSize()
{
	GLint data;
	glGetNamedBufferParameteriv(VAO, GL_BUFFER_SIZE, &data);
	return data;
}*/

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
