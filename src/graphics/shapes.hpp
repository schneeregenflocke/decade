#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <cstddef>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"

class QuadrilateralShape : public Shape {
 public:
  explicit QuadrilateralShape(Shader* shader_ptr_in) : Shape(shader_ptr_in) {}

  void SetShape(const rectf& rectangle) {
    constexpr size_t kVerticesPerQuad = 6;
    std::vector<glm::vec3> vertices(kVerticesPerQuad);

    constexpr float kZero = 0.0F;
    vertices[0] = glm::vec3(rectangle.l(), rectangle.b(), kZero);
    vertices[1] = glm::vec3(rectangle.r(), rectangle.b(), kZero);
    vertices[2] = glm::vec3(rectangle.l(), rectangle.t(), kZero);
    vertices[3] = glm::vec3(rectangle.r(), rectangle.t(), kZero);
    vertices[4] = glm::vec3(rectangle.l(), rectangle.t(), kZero);
    vertices[kVerticesPerQuad - 1] =
        glm::vec3(rectangle.r(), rectangle.b(), kZero);

    SetBuffer(BufferIndex{0}, static_cast<GLsizei>(vertices.size()),
              vertices.data());
  }

  void SetColor(const glm::vec4& new_color) { color_ = new_color; }

  void Draw(const glm::mat4& model) const override {
    GetShader()->UseProgram();
    GetShader()->SetUniform("model", model);
    GetShader()->SetUniform("color", color_);

    VaoRef().Bind();
    glDrawArrays(GL_TRIANGLES, 0, VertexCount());
    VertexArrayObject::Unbind();
  }

 private:
  glm::vec4 color_{0.0F, 0.0F, 0.0F, 1.0F};
};

class RectanglesShape : public Shape {
 public:
  explicit RectanglesShape(Shader* shader_ptr_in) : Shape(shader_ptr_in) {}

  void SetShape(const std::vector<rectf>& rectangles, float line_width) {
    constexpr size_t kVerticesPerQuad = 6;
    constexpr size_t kQuadsPerRectangle = 5;
    constexpr size_t kVerticesPerRectangle =
        kVerticesPerQuad * kQuadsPerRectangle;
    const size_t size = rectangles.size() * kVerticesPerRectangle;
    vertices_.resize(size);

    for (size_t index = 0; index < rectangles.size(); ++index) {
      SetRectangleShape(index, rectangles[index], line_width);
    }

    SetBuffer(BufferIndex{0}, static_cast<GLsizei>(vertices_.size()),
              vertices_.data());
  }

  void SetShape(const rectf& rectangle, float line_width) {
    constexpr size_t kVerticesPerQuad = 6;
    constexpr size_t kQuadsPerRectangle = 5;
    constexpr size_t kVerticesPerRectangle =
        kVerticesPerQuad * kQuadsPerRectangle;
    vertices_.resize(kVerticesPerRectangle);

    SetRectangleShape(0, rectangle, line_width);

    SetBuffer(BufferIndex{0}, static_cast<GLsizei>(vertices_.size()),
              vertices_.data());
  }

  void SetColor(const std::vector<glm::vec4>& new_colors) {
    colors_ = new_colors;
  }

  void Draw(const glm::mat4& model) const override {
    GetShader()->UseProgram();
    GetShader()->SetUniform("model", model);

    if (colors_.size() == 2) {
      GetShader()->SetUniform("outline_color", colors_[0]);
      GetShader()->SetUniform("fill_color", colors_[1]);
    }

    VaoRef().Bind();
    glDrawArrays(GL_TRIANGLES, 0, VertexCount());
    VertexArrayObject::Unbind();
  }

 private:
  void SetRectangleShape(size_t index, const rectf& rectangle,
                         float line_width) {
    constexpr size_t kVerticesPerQuad = 6;
    constexpr size_t kQuadsPerRectangle = 5;
    constexpr size_t kVerticesPerRectangle =
        kVerticesPerQuad * kQuadsPerRectangle;

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
    constexpr size_t kVerticesPerQuad = 6;
    vertices_[offset + 0] = point0;
    vertices_[offset + 1] = point1;
    vertices_[offset + 2] = point2;
    vertices_[offset + 3] = point3;
    vertices_[offset + 4] = point2;
    vertices_[offset + (kVerticesPerQuad - 1)] = point1;
  }

  std::vector<glm::vec3> vertices_;
  std::vector<glm::vec4> colors_;
};
#endif  // SHAPES_HPP
