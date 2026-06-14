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

  void set_shape(const rectf& rectangle) {
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

    set_buffer(BufferIndex{0}, static_cast<GLsizei>(vertices.size()),
               vertices.data());
  }

  void set_color(const glm::vec4& new_color) { color_ = new_color; }

  void draw(const glm::mat4& model) const override {
    shader()->UseProgram();
    shader()->SetUniform("model", model);
    shader()->SetUniform("color", color_);

    vao_ref().bind();
    glDrawArrays(GL_TRIANGLES, 0, vertex_count());
    VertexArrayObject::Unbind();
  }

 private:
  glm::vec4 color_{0.0F, 0.0F, 0.0F, 1.0F};
};

class RectanglesShape : public Shape {
 public:
  explicit RectanglesShape(Shader* shader_ptr_in) : Shape(shader_ptr_in) {}

  void set_shape(const std::vector<rectf>& rectangles, float line_width) {
    constexpr size_t kVerticesPerQuad = 6;
    constexpr size_t kQuadsPerRectangle = 5;
    constexpr size_t kVerticesPerRectangle =
        kVerticesPerQuad * kQuadsPerRectangle;
    const size_t size = rectangles.size() * kVerticesPerRectangle;
    vertices_.resize(size);

    for (size_t index = 0; index < rectangles.size(); ++index) {
      set_rectangle_shape(index, rectangles[index], line_width);
    }

    set_buffer(BufferIndex{0}, static_cast<GLsizei>(vertices_.size()),
               vertices_.data());
  }

  void set_shape(const rectf& rectangle, float line_width) {
    constexpr size_t kVerticesPerQuad = 6;
    constexpr size_t kQuadsPerRectangle = 5;
    constexpr size_t kVerticesPerRectangle =
        kVerticesPerQuad * kQuadsPerRectangle;
    vertices_.resize(kVerticesPerRectangle);

    set_rectangle_shape(0, rectangle, line_width);

    set_buffer(BufferIndex{0}, static_cast<GLsizei>(vertices_.size()),
               vertices_.data());
  }

  void set_color(const std::vector<glm::vec4>& new_colors) {
    colors_ = new_colors;
  }

  void draw(const glm::mat4& model) const override {
    shader()->UseProgram();
    shader()->SetUniform("model", model);

    if (colors_.size() == 2) {
      shader()->SetUniform("outline_color", colors_[0]);
      shader()->SetUniform("fill_color", colors_[1]);
    }

    vao_ref().bind();
    glDrawArrays(GL_TRIANGLES, 0, vertex_count());
    VertexArrayObject::Unbind();
  }

 private:
  void set_rectangle_shape(size_t index, const rectf& rectangle,
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
    set_rectangle(offset, inrectangle.getLB(), inrectangle.getRB(),
                  inrectangle.getLT(), inrectangle.getRT());
    // top outline
    set_rectangle(offset + kVerticesPerQuad, inrectangle.getLT(),
                  inrectangle.getRT(), outrectangle.getLT(),
                  outrectangle.getRT());
    // bottom outline
    set_rectangle(offset + (kVerticesPerQuad * 2), outrectangle.getLB(),
                  outrectangle.getRB(), inrectangle.getLB(),
                  inrectangle.getRB());
    // left outline
    set_rectangle(offset + (kVerticesPerQuad * 3), outrectangle.getLB(),
                  inrectangle.getLB(), outrectangle.getLT(),
                  inrectangle.getLT());
    // right outline
    set_rectangle(offset + (kVerticesPerQuad * 4), inrectangle.getRB(),
                  outrectangle.getRB(), inrectangle.getRT(),
                  outrectangle.getRT());
  }

  void set_rectangle(size_t offset, const glm::vec3& point0,
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
