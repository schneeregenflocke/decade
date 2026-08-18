#ifndef SHAPES_BASE_HPP
#define SHAPES_BASE_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <span>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shaders_info.hpp"
#include "vertex_objects.hpp"

class Shape : public Drawable {
 public:
  struct BufferIndex {
    size_t value;
  };

  // A shape without a shader cannot exist — SetShader dereferences at once —
  // so the type says it instead of a null check that nobody would reach.
  explicit Shape(Shader& shader_in);

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

  void Draw(const glm::mat4& model) const override;

  // Recorded by each concrete shape when its geometry is set. Used for spatial
  // queries (the scene-tree selection highlight) without exposing the buffers.
  [[nodiscard]] const RectF& LocalBounds() const override;

 protected:
  [[nodiscard]] GLsizei VertexCount() const;
  [[nodiscard]] Shader& GetShader() const;
  [[nodiscard]] const VertexArrayObject& VaoRef() const;
  void SetLocalBounds(const RectF& bounds);

 private:
  void SetUpBuffers();

  Shader& shader_;
  GLsizei number_vertices_{0};
  VertexArrayObject vao_;
  std::vector<VertexBufferObject> vbos_;
  std::vector<ShaderInfo> attributes_infos_;
  RectF local_bounds_;
};

#endif  // SHAPES_BASE_HPP
