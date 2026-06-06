#ifndef SHAPES_BASE_HPP
#define SHAPES_BASE_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <utility>
#include <vector>

#include "shaders.hpp"
#include "shaders_info.hpp"

inline void PrintError() {
  const GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cout << "OpenGL Error: " << std::hex << error << std::dec << '\n';
  }
}

class VertexArrayObject {
 public:
  VertexArrayObject() { glCreateVertexArrays(1, &vao); }

  ~VertexArrayObject() { glDeleteVertexArrays(1, &vao); }

  VertexArrayObject(const VertexArrayObject&) = delete;
  VertexArrayObject& operator=(const VertexArrayObject&) = delete;

  VertexArrayObject(VertexArrayObject&& other) noexcept
      : vao(std::exchange(other.vao, 0)) {}
  VertexArrayObject& operator=(VertexArrayObject&& other) noexcept {
    if (this != &other) {
      if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
      }
      vao = std::exchange(other.vao, 0);
    }
    return *this;
  }

  [[nodiscard]] bool is_valid() const { return glIsVertexArray(vao); }

  void bind() const { glBindVertexArray(vao); }

  static void Unbind() { glBindVertexArray(0); }

  [[nodiscard]] GLuint get() const { return vao; }

 private:
  GLuint vao{0};
};

class VertexBufferObject {
 public:
  VertexBufferObject() { glCreateBuffers(1, &vbo); }

  ~VertexBufferObject() { glDeleteBuffers(1, &vbo); }

  VertexBufferObject(const VertexBufferObject&) = delete;
  VertexBufferObject& operator=(const VertexBufferObject&) = delete;

  VertexBufferObject(VertexBufferObject&& other) noexcept
      : vbo(std::exchange(other.vbo, 0)) {}
  VertexBufferObject& operator=(VertexBufferObject&& other) noexcept {
    if (this != &other) {
      if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
      }
      vbo = std::exchange(other.vbo, 0);
    }
    return *this;
  }

  void bind() const { glBindBuffer(GL_ARRAY_BUFFER, vbo); }

  static void Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  [[nodiscard]] GLuint get() const { return vbo; }

 private:
  GLuint vbo{0};
};

class Shape {
 public:
  struct BufferIndex {
    size_t value;
  };

  explicit Shape(Shader* shader_ptr_in) { set_shader(shader_ptr_in); }
  virtual ~Shape() = default;

  Shape(const Shape&) = delete;
  Shape& operator=(const Shape&) = delete;
  Shape(Shape&&) = delete;
  Shape& operator=(Shape&&) = delete;

  void set_buffer(BufferIndex index, GLsizei vertex_count, const void* data) {
    number_vertices = vertex_count;

    const auto& attribute_info = attributes_infos.at(index.value);
    const auto type_size =
        static_cast<GLsizeiptr>(attribute_info.GetTypeSize());
    const auto buffer_size = static_cast<GLsizeiptr>(vertex_count) * type_size;

    vao.bind();
    vbos.at(index.value).bind();

    glBufferData(GL_ARRAY_BUFFER, buffer_size, data, GL_DYNAMIC_DRAW);
    // glNamedBufferData(vbos[index].get(), buffer_size, data, GL_DYNAMIC_DRAW);

    VertexBufferObject::Unbind();
    VertexArrayObject::Unbind();
  }

  virtual void draw() const {
    shader_ptr->UseProgram();
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices);
    VertexArrayObject::Unbind();
  }

  [[nodiscard]] Shader* get_shader() const { return shader_ptr; }

 protected:
  [[nodiscard]] GLsizei vertex_count() const { return number_vertices; }
  [[nodiscard]] Shader* shader() const { return shader_ptr; }
  [[nodiscard]] VertexArrayObject& vao_ref() { return vao; }
  [[nodiscard]] const VertexArrayObject& vao_ref() const { return vao; }

 private:
  void set_shader(Shader* new_shader_ptr) {
    shader_ptr = new_shader_ptr;
    attributes_infos = new_shader_ptr->GetShaderAttributesInfos();

    vao.bind();

    vbos.resize(attributes_infos.size());

    for (size_t index = 0; index < attributes_infos.size(); ++index) {
      const auto& attribute_info = attributes_infos[index];

      vbos[index].bind();

      const auto attribute_location =
          static_cast<GLuint>(attribute_info.GetLocation());

      // glVertexArrayAttribFormat(vao.get(), attribute_info.location,
      // attribute_info.number, GL_FLOAT, GL_FALSE, 0);
      glVertexAttribFormat(attribute_location,
                           static_cast<GLint>(attribute_info.GetNumber()),
                           GL_FLOAT, GL_FALSE, 0);

      const auto binding_index = static_cast<GLuint>(index);

      // glVertexArrayAttribBinding(vao.get(), attribute_info.location, index);
      glVertexAttribBinding(attribute_location, binding_index);

      // glVertexArrayVertexBuffer(vao.get(), index, vbos[index].get(), 0,
      // attribute_info.type_size);
      glBindVertexBuffer(binding_index, vbos[index].get(), 0,
                         static_cast<GLsizei>(attribute_info.GetTypeSize()));

      // glEnableVertexArrayAttrib(vao.get(), attribute_info.location);
      glEnableVertexAttribArray(attribute_location);

      VertexBufferObject::Unbind();
    }

    VertexArrayObject::Unbind();
  }

  GLsizei number_vertices{0};
  VertexArrayObject vao;
  Shader* shader_ptr{nullptr};
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
                glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr,
GL_DYNAMIC_DRAW); vbo.unbind();
        }

        void UpdateBuffer()
        {
                vbo.bind();
                glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size,
vertices.data()); vbo.unbind();
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
                vec3 subvector0 = vertices[index].point - vertices[index +
1].point; vec3 subvector1 = vertices[index].point - vertices[index + 2].point;

                vec3 cross = glm::cross(subvector0, subvector1);
                vec3 normal = glm::normalize(cross);

                vertices[index + 0].normal = normal;
                vertices[index + 1].normal = normal;
                vertices[index + 2].normal = normal;
        }
}*/
#endif  // SHAPES_BASE_HPP
