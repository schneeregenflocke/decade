#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <algorithm>
#include <cstddef>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <span>
#include <vector>

#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"

// Two triangles form a quad — the number stands here once, because both shapes
// below compute in that unit.
inline constexpr size_t kVerticesPerQuad = 6;

class QuadrilateralShape : public Shape {
 public:
  explicit QuadrilateralShape(Shader& shader_in) : Shape(shader_in) {}

  void SetShape(const rectf& rectangle) {
    std::vector<glm::vec3> vertices(kVerticesPerQuad);

    constexpr float kZero = 0.0F;
    vertices[0] = glm::vec3(rectangle.l(), rectangle.b(), kZero);
    vertices[1] = glm::vec3(rectangle.r(), rectangle.b(), kZero);
    vertices[2] = glm::vec3(rectangle.l(), rectangle.t(), kZero);
    vertices[3] = glm::vec3(rectangle.r(), rectangle.t(), kZero);
    vertices[4] = glm::vec3(rectangle.l(), rectangle.t(), kZero);
    vertices[kVerticesPerQuad - 1] =
        glm::vec3(rectangle.r(), rectangle.b(), kZero);

    SetBuffer(BufferIndex{0}, std::span<const glm::vec3>(vertices));
    SetLocalBounds(rectangle);
  }

  void SetColor(const glm::vec4& new_color) { color_ = new_color; }

  void Draw(const glm::mat4& model) const override {
    GetShader().UseProgram();
    GetShader().SetUniform("model", model);
    GetShader().SetUniform("color", color_);

    VaoRef().Bind();
    glDrawArrays(GL_TRIANGLES, 0, VertexCount());
    VertexArrayObject::Unbind();
  }

 private:
  glm::vec4 color_{0.0F, 0.0F, 0.0F, 1.0F};
};

class RectanglesShape : public Shape {
 public:
  explicit RectanglesShape(Shader& shader_in) : Shape(shader_in) {}

  void SetShape(const std::vector<rectf>& rectangles, float line_width) {
    vertices_.resize(rectangles.size() * kVerticesPerRectangle);

    for (size_t index = 0; index < rectangles.size(); ++index) {
      SetRectangleShape(index, rectangles[index], line_width);
    }

    SetBuffer(BufferIndex{0}, std::span<const glm::vec3>(vertices_));
    SetLocalBounds(UnionBounds(rectangles, line_width));
  }

  void SetShape(const rectf& rectangle, float line_width) {
    SetShape(std::vector<rectf>{rectangle}, line_width);
  }

  // Outline and fill, named instead of sitting as a pair in a vector: the
  // shader knows exactly these two uniforms.
  void SetColors(const glm::vec4& outline_color, const glm::vec4& fill_color) {
    outline_color_ = outline_color;
    fill_color_ = fill_color;
  }

  void Draw(const glm::mat4& model) const override {
    GetShader().UseProgram();
    GetShader().SetUniform("model", model);

    GetShader().SetUniform("outline_color", outline_color_);
    GetShader().SetUniform("fill_color", fill_color_);

    VaoRef().Bind();
    glDrawArrays(GL_TRIANGLES, 0, VertexCount());
    VertexArrayObject::Unbind();
  }

 private:
  // Axis-aligned union of the rectangles, grown by half the line width so the
  // box covers the drawn outline (which straddles each edge).
  static rectf UnionBounds(const std::vector<rectf>& rectangles,
                           float line_width) {
    if (rectangles.empty()) {
      return {};
    }
    const float half_line = line_width * 0.5F;
    float left = rectangles[0].l();
    float right = rectangles[0].r();
    float bottom = rectangles[0].b();
    float top = rectangles[0].t();
    for (const auto& rectangle : rectangles) {
      left = std::min(left, rectangle.l());
      right = std::max(right, rectangle.r());
      bottom = std::min(bottom, rectangle.b());
      top = std::max(top, rectangle.t());
    }
    return {left - half_line, right + half_line, bottom - half_line,
            top + half_line};
  }

  void SetRectangleShape(size_t index, const rectf& rectangle,
                         float line_width) {
    const float half_line_thickness = line_width * 0.5F;

    const rectf inrectangle =
        rectangle.reduce(rectf(half_line_thickness, half_line_thickness,
                               half_line_thickness, half_line_thickness));
    const rectf outrectangle =
        rectangle.expand(rectf(half_line_thickness, half_line_thickness,
                               half_line_thickness, half_line_thickness));

    const size_t offset = index * kVerticesPerRectangle;

    // fill
    SetRectangle(offset, inrectangle.getLB(), inrectangle.getRB(),
                 inrectangle.getLT(), inrectangle.getRT());
    // top outline
    SetRectangle(offset + kVerticesPerQuad, inrectangle.getLT(),
                 inrectangle.getRT(), outrectangle.getLT(),
                 outrectangle.getRT());
    // bottom outline
    SetRectangle(offset + (kVerticesPerQuad * 2), outrectangle.getLB(),
                 outrectangle.getRB(), inrectangle.getLB(),
                 inrectangle.getRB());
    // left outline
    SetRectangle(offset + (kVerticesPerQuad * 3), outrectangle.getLB(),
                 inrectangle.getLB(), outrectangle.getLT(),
                 inrectangle.getLT());
    // right outline
    SetRectangle(offset + (kVerticesPerQuad * 4), inrectangle.getRB(),
                 outrectangle.getRB(), inrectangle.getRT(),
                 outrectangle.getRT());
  }

  void SetRectangle(size_t offset, const glm::vec3& point0,
                    const glm::vec3& point1, const glm::vec3& point2,
                    const glm::vec3& point3) {
    vertices_[offset + 0] = point0;
    vertices_[offset + 1] = point1;
    vertices_[offset + 2] = point2;
    vertices_[offset + 3] = point3;
    vertices_[offset + 4] = point2;
    vertices_[offset + (kVerticesPerQuad - 1)] = point1;
  }

  // The fill plus four outline strips per rectangle.
  static constexpr size_t kQuadsPerRectangle = 5;
  static constexpr size_t kVerticesPerRectangle =
      kVerticesPerQuad * kQuadsPerRectangle;

  std::vector<glm::vec3> vertices_;
  glm::vec4 outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
  glm::vec4 fill_color_{0.0F, 0.0F, 0.0F, 1.0F};
};
#endif  // SHAPES_HPP
