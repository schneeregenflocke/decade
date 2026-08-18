#include "shapes.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"
#include "vertex_objects.hpp"

FillShape::FillShape(Shader& shader_in) : Shape(shader_in) {}

DrawableKind FillShape::Kind() const { return DrawableKind::kFill; }

void FillShape::SetShape(const RectF& rectangle) {
  std::vector<glm::vec3> vertices(kVerticesPerQuad);

  constexpr float kZero = 0.0F;
  vertices[0] = glm::vec3(rectangle.Left(), rectangle.Bottom(), kZero);
  vertices[1] = glm::vec3(rectangle.Right(), rectangle.Bottom(), kZero);
  vertices[2] = glm::vec3(rectangle.Left(), rectangle.Top(), kZero);
  vertices[3] = glm::vec3(rectangle.Right(), rectangle.Top(), kZero);
  vertices[4] = glm::vec3(rectangle.Left(), rectangle.Top(), kZero);
  vertices[kVerticesPerQuad - 1] =
      glm::vec3(rectangle.Right(), rectangle.Bottom(), kZero);

  SetBuffer(BufferIndex{0}, std::span<const glm::vec3>(vertices));
  SetLocalBounds(rectangle);
}

void FillShape::SetColor(const glm::vec4& new_color) { color_ = new_color; }

void FillShape::Draw(const glm::mat4& model) const {
  GetShader().UseProgram();
  GetShader().SetUniform("model", model);
  GetShader().SetUniform("color", color_);

  VaoRef().Bind();
  glDrawArrays(GL_TRIANGLES, 0, VertexCount());
  VertexArrayObject::Unbind();
}

BoxesShape::BoxesShape(Shader& shader_in) : Shape(shader_in) {}

DrawableKind BoxesShape::Kind() const { return DrawableKind::kBoxes; }

void BoxesShape::SetShape(const std::vector<RectF>& rectangles,
                          float line_width) {
  vertices_.resize(rectangles.size() * kVerticesPerRectangle);

  for (size_t index = 0; index < rectangles.size(); ++index) {
    SetRectangleShape(index, rectangles[index], line_width);
  }

  SetBuffer(BufferIndex{0}, std::span<const glm::vec3>(vertices_));
  SetLocalBounds(UnionBounds(rectangles, line_width));
}

void BoxesShape::SetShape(const RectF& rectangle, float line_width) {
  SetShape(std::vector<RectF>{rectangle}, line_width);
}

void BoxesShape::SetColors(const glm::vec4& outline_color,
                           const glm::vec4& fill_color) {
  outline_color_ = outline_color;
  fill_color_ = fill_color;
}

void BoxesShape::Draw(const glm::mat4& model) const {
  GetShader().UseProgram();
  GetShader().SetUniform("model", model);

  GetShader().SetUniform("outline_color", outline_color_);
  GetShader().SetUniform("fill_color", fill_color_);

  VaoRef().Bind();
  glDrawArrays(GL_TRIANGLES, 0, VertexCount());
  VertexArrayObject::Unbind();
}

RectF BoxesShape::UnionBounds(const std::vector<RectF>& rectangles,
                              float line_width) {
  if (rectangles.empty()) {
    return {};
  }
  const float half_line = line_width * 0.5F;
  float left = rectangles[0].Left();
  float right = rectangles[0].Right();
  float bottom = rectangles[0].Bottom();
  float top = rectangles[0].Top();
  for (const auto& rectangle : rectangles) {
    left = std::min(left, rectangle.Left());
    right = std::max(right, rectangle.Right());
    bottom = std::min(bottom, rectangle.Bottom());
    top = std::max(top, rectangle.Top());
  }
  return {left - half_line, right + half_line, bottom - half_line,
          top + half_line};
}

void BoxesShape::SetRectangleShape(size_t index, const RectF& rectangle,
                                   float line_width) {
  const float half_line_thickness = line_width * 0.5F;

  const RectF inrectangle =
      rectangle.Reduce(RectF(half_line_thickness, half_line_thickness,
                             half_line_thickness, half_line_thickness));
  const RectF outrectangle =
      rectangle.Expand(RectF(half_line_thickness, half_line_thickness,
                             half_line_thickness, half_line_thickness));

  const size_t offset = index * kVerticesPerRectangle;

  // fill
  SetRectangle(offset, inrectangle.LeftBottom(), inrectangle.RightBottom(),
               inrectangle.LeftTop(), inrectangle.RightTop());
  // top outline
  SetRectangle(offset + kVerticesPerQuad, inrectangle.LeftTop(),
               inrectangle.RightTop(), outrectangle.LeftTop(),
               outrectangle.RightTop());
  // bottom outline
  SetRectangle(offset + (kVerticesPerQuad * 2), outrectangle.LeftBottom(),
               outrectangle.RightBottom(), inrectangle.LeftBottom(),
               inrectangle.RightBottom());
  // left outline
  SetRectangle(offset + (kVerticesPerQuad * 3), outrectangle.LeftBottom(),
               inrectangle.LeftBottom(), outrectangle.LeftTop(),
               inrectangle.LeftTop());
  // right outline
  SetRectangle(offset + (kVerticesPerQuad * 4), inrectangle.RightBottom(),
               outrectangle.RightBottom(), inrectangle.RightTop(),
               outrectangle.RightTop());
}

void BoxesShape::SetRectangle(size_t offset, const glm::vec3& point0,
                              const glm::vec3& point1, const glm::vec3& point2,
                              const glm::vec3& point3) {
  vertices_[offset + 0] = point0;
  vertices_[offset + 1] = point1;
  vertices_[offset + 2] = point2;
  vertices_[offset + 3] = point3;
  vertices_[offset + 4] = point2;
  vertices_[offset + (kVerticesPerQuad - 1)] = point1;
}
