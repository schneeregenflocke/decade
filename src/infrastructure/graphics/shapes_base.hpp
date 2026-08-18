#ifndef SHAPES_BASE_HPP
#define SHAPES_BASE_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <span>
#include <string_view>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shaders_info.hpp"
#include "vertex_objects.hpp"

class Shape : public Drawable {
 public:
  // A shape without a shader cannot exist — SetUpBuffers reads the attribute
  // layout out of it at once — so the type says it instead of a null check that
  // nobody would reach.
  explicit Shape(Shader& shader_in);

  // The vertices arrive as one span rather than as a pointer beside a count,
  // so the two cannot disagree at a call site. The attribute is addressed by
  // the name it carries in the shader, the way SetUniform addresses a uniform:
  // the layout comes out of the linked program, and a position in that list
  // shifts as soon as the driver drops an attribute the shader never reads.
  template <typename Vertex>
  void SetBuffer(std::string_view attribute_name,
                 std::span<const Vertex> vertices) {
    const std::size_t index = BufferIndexFor(attribute_name, sizeof(Vertex));
    number_vertices_ = static_cast<GLsizei>(vertices.size());

    glNamedBufferData(vbos_[index].Name(),
                      static_cast<GLsizeiptr>(vertices.size_bytes()),
                      vertices.data(), GL_DYNAMIC_DRAW);
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
  // The buffer feeding `attribute_name`, with the vertex type checked against
  // what the shader declares. Both failures throw and name the shader: an
  // attribute the program does not carry, and one of a different width — the
  // latter would otherwise upload the wrong number of bytes out of the span.
  [[nodiscard]] std::size_t BufferIndexFor(std::string_view attribute_name,
                                           std::size_t vertex_size) const;

  void SetUpBuffers();

  Shader& shader_;
  GLsizei number_vertices_{0};
  VertexArrayObject vao_;
  std::vector<VertexBufferObject> vbos_;
  std::vector<ShaderInfo> attributes_infos_;
  RectF local_bounds_;
};

#endif  // SHAPES_BASE_HPP
