#ifndef SHAPES_BASE_HPP
#define SHAPES_BASE_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <span>
#include <utility>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"
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

class Shape : public Drawable {
 public:
  struct BufferIndex {
    size_t value;
  };

  // A shape without a shader cannot exist — SetShader dereferences at once —
  // so the type says it instead of a null check that nobody would reach.
  explicit Shape(Shader& shader_in) : shader_(shader_in) { SetUpBuffers(); }

  // The vertices arrive as one span rather than as a pointer beside a count,
  // so the two cannot disagree at a call site. The byte size still comes from
  // the shader attribute and not from `sizeof(T)`: the attribute decides how
  // wide a vertex is on the GL side, and that is what glBufferData is told.
  template <typename Vertex>
  void SetBuffer(BufferIndex index, std::span<const Vertex> vertices) {
    number_vertices_ = static_cast<GLsizei>(vertices.size());

    const auto& attribute_info = attributes_infos_.at(index.value);
    const auto type_size =
        static_cast<GLsizeiptr>(attribute_info.GetTypeSize());
    const auto buffer_size =
        static_cast<GLsizeiptr>(vertices.size()) * type_size;
    const void* data = vertices.data();

    vao_.Bind();
    vbos_.at(index.value).Bind();

    glBufferData(GL_ARRAY_BUFFER, buffer_size, data, GL_DYNAMIC_DRAW);

    VertexBufferObject::Unbind();
    VertexArrayObject::Unbind();
  }

  void Draw(const glm::mat4& model) const override {
    shader_.UseProgram();
    shader_.SetUniform("model", model);
    vao_.Bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices_);
    VertexArrayObject::Unbind();
  }

  // Recorded by each concrete shape when its geometry is set. Used for spatial
  // queries (the scene-tree selection highlight) without exposing the buffers.
  [[nodiscard]] const RectF& LocalBounds() const override {
    return local_bounds_;
  }

 protected:
  [[nodiscard]] GLsizei VertexCount() const { return number_vertices_; }
  [[nodiscard]] Shader& GetShader() const { return shader_; }
  [[nodiscard]] VertexArrayObject& VaoRef() { return vao_; }
  [[nodiscard]] const VertexArrayObject& VaoRef() const { return vao_; }
  void SetLocalBounds(const RectF& bounds) { local_bounds_ = bounds; }

 private:
  void SetUpBuffers() {
    attributes_infos_ = shader_.GetShaderAttributesInfos();

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

  Shader& shader_;
  GLsizei number_vertices_{0};
  VertexArrayObject vao_;
  std::vector<VertexBufferObject> vbos_;
  std::vector<ShaderInfo> attributes_infos_;
  RectF local_bounds_;
};

#endif  // SHAPES_BASE_HPP
