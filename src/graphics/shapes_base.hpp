#ifndef SHAPES_BASE_HPP
#define SHAPES_BASE_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <utility>
#include <vector>

#include "shaders.hpp"
#include "shaders_info.hpp"

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

  void Bind() const { glBindVertexArray(vao_); }

  static void Unbind() { glBindVertexArray(0); }

  [[nodiscard]] GLuint Get() const { return vao_; }

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

  void Bind() const { glBindBuffer(GL_ARRAY_BUFFER, vbo_); }

  static void Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  [[nodiscard]] GLuint Get() const { return vbo_; }

 private:
  GLuint vbo_{0};
};

class Shape {
 public:
  struct BufferIndex {
    size_t value;
  };

  explicit Shape(Shader* shader_ptr_in) { SetShader(shader_ptr_in); }
  virtual ~Shape() = default;

  Shape(const Shape&) = delete;
  Shape& operator=(const Shape&) = delete;
  Shape(Shape&&) = delete;
  Shape& operator=(Shape&&) = delete;

  void SetBuffer(BufferIndex index, GLsizei vertex_count, const void* data) {
    number_vertices_ = vertex_count;

    const auto& attribute_info = attributes_infos_.at(index.value);
    const auto type_size =
        static_cast<GLsizeiptr>(attribute_info.GetTypeSize());
    const auto buffer_size = static_cast<GLsizeiptr>(vertex_count) * type_size;

    vao_.Bind();
    vbos_.at(index.value).Bind();

    glBufferData(GL_ARRAY_BUFFER, buffer_size, data, GL_DYNAMIC_DRAW);

    VertexBufferObject::Unbind();
    VertexArrayObject::Unbind();
  }

  virtual void Draw(const glm::mat4& model) const {
    shader_ptr_->UseProgram();
    shader_ptr_->SetUniform("model", model);
    vao_.Bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices_);
    VertexArrayObject::Unbind();
  }

 protected:
  [[nodiscard]] GLsizei VertexCount() const { return number_vertices_; }
  [[nodiscard]] Shader* GetShader() const { return shader_ptr_; }
  [[nodiscard]] VertexArrayObject& VaoRef() { return vao_; }
  [[nodiscard]] const VertexArrayObject& VaoRef() const { return vao_; }

 private:
  void SetShader(Shader* new_shader_ptr) {
    shader_ptr_ = new_shader_ptr;
    attributes_infos_ = new_shader_ptr->GetShaderAttributesInfos();

    vao_.Bind();

    vbos_.resize(attributes_infos_.size());

    for (size_t index = 0; index < attributes_infos_.size(); ++index) {
      const auto& attribute_info = attributes_infos_[index];

      vbos_[index].Bind();

      const auto attribute_location =
          static_cast<GLuint>(attribute_info.GetLocation());

      glVertexAttribFormat(attribute_location,
                           static_cast<GLint>(attribute_info.GetNumber()),
                           GL_FLOAT, GL_FALSE, 0);

      const auto binding_index = static_cast<GLuint>(index);

      glVertexAttribBinding(attribute_location, binding_index);

      glBindVertexBuffer(binding_index, vbos_[index].Get(), 0,
                         static_cast<GLsizei>(attribute_info.GetTypeSize()));

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

#endif  // SHAPES_BASE_HPP
