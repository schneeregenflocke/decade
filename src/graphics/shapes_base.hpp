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
#include "shaders_info.hpp"

#include <iostream>
// #include <variant>
#include <vector>

// #define GLM_FORCE_MESSAGES
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <glad/glad.h>

void PrintError()
{
  GLenum error = glGetError();
  if (error != GL_NO_ERROR)
    std::cout << "OpenGL Error: " << std::hex << error << std::dec << std::endl;
}

class VertexArrayObject {
public:
  explicit VertexArrayObject() { glCreateVertexArrays(1, &vao); }

  virtual ~VertexArrayObject() { glDeleteVertexArrays(1, &vao); }

  bool is_valid() const { return glIsVertexArray(vao); }

  void bind() const { glBindVertexArray(vao); }

  void unbind() const { glBindVertexArray(0); }

  GLuint get() const { return vao; }

private:
  GLuint vao;
};

class VertexBufferObject {
public:
  explicit VertexBufferObject() { glCreateBuffers(1, &vbo); }

  virtual ~VertexBufferObject() { glDeleteBuffers(1, &vbo); }

  void bind() const { glBindBuffer(GL_ARRAY_BUFFER, vbo); }

  void unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  GLuint get() const { return vbo; }

private:
  GLuint vbo;
};

class Shape {
public:
  explicit Shape(Shader *shader_ptr) : number_vertices(0) { set_shader(shader_ptr); }

  void set_buffer(size_t index, GLsizeiptr number_vertices, const void *data)
  {
    this->number_vertices = number_vertices;

    GLsizeiptr type_size = attributes_infos[index].type_size;
    GLsizeiptr buffer_size = number_vertices * type_size;

    auto data_pointer_size = sizeof(data);

    vao.bind();
    vbos[index].bind();

    glBufferData(GL_ARRAY_BUFFER, buffer_size, data, GL_DYNAMIC_DRAW);
    // glNamedBufferData(vbos[index].get(), buffer_size, data, GL_DYNAMIC_DRAW);

    vbos[index].unbind();
    vao.unbind();
  }

  virtual void draw() const
  {
    shader_ptr->UseProgram();
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices);
    vao.unbind();
  }

  Shader *get_shader() const { return shader_ptr; }

protected:
  GLsizei number_vertices;
  VertexArrayObject vao;
  Shader *shader_ptr;

private:
  void set_shader(Shader *shader_ptr)
  {
    this->shader_ptr = shader_ptr;
    attributes_infos = shader_ptr->GetShaderAttributesInfos();

    vao.bind();

    vbos.resize(attributes_infos.size());

    for (size_t index = 0; index < attributes_infos.size(); ++index) {
      const auto &attribute_info = attributes_infos[index];

      vbos[index].bind();

      // glVertexArrayAttribFormat(vao.get(), attribute_info.location, attribute_info.number,
      // GL_FLOAT, GL_FALSE, 0);
      glVertexAttribFormat(attribute_info.location, attribute_info.number, GL_FLOAT, GL_FALSE, 0);

      GLuint binding_index = index;

      // glVertexArrayAttribBinding(vao.get(), attribute_info.location, index);
      glVertexAttribBinding(attribute_info.location, binding_index);

      // glVertexArrayVertexBuffer(vao.get(), index, vbos[index].get(), 0,
      // attribute_info.type_size);
      glBindVertexBuffer(binding_index, vbos[index].get(), 0, attribute_info.type_size);

      // glEnableVertexArrayAttrib(vao.get(), attribute_info.location);
      glEnableVertexAttribArray(attribute_info.location);

      vbos[index].unbind();
    }

    vao.unbind();
  }

  std::vector<VertexBufferObject> vbos;
  std::vector<ShaderInfo> attributes_infos;
};

/*template<typename T>
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
                vao.bind();
                U::EnableVertexAttribArrays();

                vbo.bind();
                U::SetAttribPointers(vbo.get());

                glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

                vao.unbind();
                vbo.unbind();
        }

        void SetBufferSize(GLsizeiptr vertices_size)
        {
                this->vertices_size = vertices_size;
                buffer_size = vertices_size * sizeof(U);

                vertices.resize(vertices_size);

                vbo.bind();
                glNamedBufferData
                glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
                vbo.unbind();
        }

        void UpdateBuffer()
        {
                vbo.bind();
                glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, vertices.data());
                vbo.unbind();
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
};*/

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
