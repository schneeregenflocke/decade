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
  VertexArrayObject() { glCreateVertexArrays(1, &vao_); }

  ~VertexArrayObject() { glDeleteVertexArrays(1, &vao_); }

  VertexArrayObject(const VertexArrayObject&) = delete;
  VertexArrayObject& operator=(const VertexArrayObject&) = delete;

  VertexArrayObject(VertexArrayObject&& other) noexcept
      : vao_(std::exchange(other.vao_, 0)) {}
  VertexArrayObject& operator=(VertexArrayObject&& other) noexcept {
    if (this != &other) {
      if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
      }
      vao_ = std::exchange(other.vao_, 0);
    }
    return *this;
  }

  [[nodiscard]] bool is_valid() const { return glIsVertexArray(vao_); }

  void bind() const { glBindVertexArray(vao_); }

  static void Unbind() { glBindVertexArray(0); }

  [[nodiscard]] GLuint get() const { return vao_; }

 private:
  GLuint vao_{0};
};

class VertexBufferObject {
 public:
  VertexBufferObject() { glCreateBuffers(1, &vbo_); }

  ~VertexBufferObject() { glDeleteBuffers(1, &vbo_); }

  VertexBufferObject(const VertexBufferObject&) = delete;
  VertexBufferObject& operator=(const VertexBufferObject&) = delete;

  VertexBufferObject(VertexBufferObject&& other) noexcept
      : vbo_(std::exchange(other.vbo_, 0)) {}
  VertexBufferObject& operator=(VertexBufferObject&& other) noexcept {
    if (this != &other) {
      if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
      }
      vbo_ = std::exchange(other.vbo_, 0);
    }
    return *this;
  }

  void bind() const { glBindBuffer(GL_ARRAY_BUFFER, vbo_); }

  static void Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  [[nodiscard]] GLuint get() const { return vbo_; }

 private:
  GLuint vbo_{0};
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
    number_vertices_ = vertex_count;

    const auto& attribute_info = attributes_infos_.at(index.value);
    const auto type_size =
        static_cast<GLsizeiptr>(attribute_info.GetTypeSize());
    const auto buffer_size = static_cast<GLsizeiptr>(vertex_count) * type_size;

    vao_.bind();
    vbos_.at(index.value).bind();

    glBufferData(GL_ARRAY_BUFFER, buffer_size, data, GL_DYNAMIC_DRAW);
    // glNamedBufferData(vbos[index].get(), buffer_size, data, GL_DYNAMIC_DRAW);

    VertexBufferObject::Unbind();
    VertexArrayObject::Unbind();
  }

  virtual void draw() const {
    shader_ptr_->UseProgram();
    vao_.bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices_);
    VertexArrayObject::Unbind();
  }

  [[nodiscard]] Shader* get_shader() const { return shader_ptr_; }

 protected:
  [[nodiscard]] GLsizei vertex_count() const { return number_vertices_; }
  [[nodiscard]] Shader* shader() const { return shader_ptr_; }
  [[nodiscard]] VertexArrayObject& vao_ref() { return vao_; }
  [[nodiscard]] const VertexArrayObject& vao_ref() const { return vao_; }

 private:
  void set_shader(Shader* new_shader_ptr) {
    shader_ptr_ = new_shader_ptr;
    attributes_infos_ = new_shader_ptr->GetShaderAttributesInfos();

    vao_.bind();

    vbos_.resize(attributes_infos_.size());

    for (size_t index = 0; index < attributes_infos_.size(); ++index) {
      const auto& attribute_info = attributes_infos_[index];

      vbos_[index].bind();

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
      glBindVertexBuffer(binding_index, vbos_[index].get(), 0,
                         static_cast<GLsizei>(attribute_info.GetTypeSize()));

      // glEnableVertexArrayAttrib(vao.get(), attribute_info.location);
      glEnableVertexAttribArray(attribute_location);

      VertexBufferObject::Unbind();
    }

    VertexArrayObject::Unbind();
  }

  GLsizei number_vertices_{0};
  VertexArrayObject vao_;
  Shader* shader_ptr_{nullptr};
  std::vector<VertexBufferObject> vbos_;
  std::vector<ShaderInfo> attributes_infos_;
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
