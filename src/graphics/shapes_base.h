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

#ifndef SHAPES_BASE_H
#define SHAPES_BASE_H

#include "shaders.h"

#include <vector>
#include <iostream>


//#define GLM_FORCE_MESSAGES
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

//using vec2 = glm::vec2 ;
//using vec3 = glm::vec3 ;
//using vec4 = glm::vec4 ;


//#define GLEW_STATIC
//#include <GL/glew.h>
#include <glad/glad.h>

//#include <lodepng.h>



class VertexArrayObject
{
public:
	VertexArrayObject();
	virtual ~VertexArrayObject();
protected:
	GLuint VAO;
};


class VertexBufferObject
{
public:
	VertexBufferObject();
	virtual ~VertexBufferObject();
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
		glDrawArrays(GL_TRIANGLES, 0, vertices_size);
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

	Shape()
		: buffersize(0)
	{
		InitializeBuffer();
	}
	
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

#endif /*SHAPES_BASE_H*/
