#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"

// Two triangles form a quad — the number stands here once, because both shapes
// below compute in that unit.
inline constexpr size_t kVerticesPerQuad = 6;

// One axis-aligned rectangle as a plain coloured area: no outline, one colour.
class FillShape : public Shape {
 public:
  explicit FillShape(Shader& shader_in);

  [[nodiscard]] DrawableKind Kind() const override;

  void SetShape(const RectF& rectangle);

  void SetColor(const glm::vec4& new_color);

  void Draw(const glm::mat4& model) const override;

 private:
  glm::vec4 color_{0.0F, 0.0F, 0.0F, 1.0F};
};

// Many axis-aligned rectangles at once, each with an outline of a given width
// around its fill — the two carry colours of their own. One draw call for all
// of them, which is why the whole set travels together.
class BoxesShape : public Shape {
 public:
  explicit BoxesShape(Shader& shader_in);

  [[nodiscard]] DrawableKind Kind() const override;

  void SetShape(const std::vector<RectF>& rectangles, float line_width);

  void SetShape(const RectF& rectangle, float line_width);

  // Outline and fill, named instead of sitting as a pair in a vector: the
  // shader knows exactly these two uniforms.
  void SetColors(const glm::vec4& outline_color, const glm::vec4& fill_color);

  void Draw(const glm::mat4& model) const override;

 private:
  // Axis-aligned union of the rectangles, grown by half the line width so the
  // box covers the drawn outline (which straddles each edge).
  static RectF UnionBounds(const std::vector<RectF>& rectangles,
                           float line_width);

  void SetRectangleShape(size_t index, const RectF& rectangle,
                         float line_width);

  void SetRectangle(size_t offset, const glm::vec3& point0,
                    const glm::vec3& point1, const glm::vec3& point2,
                    const glm::vec3& point3);

  // The fill plus four outline strips per rectangle.
  static constexpr size_t kQuadsPerRectangle = 5;
  static constexpr size_t kVerticesPerRectangle =
      kVerticesPerQuad * kQuadsPerRectangle;

  std::vector<glm::vec3> vertices_;
  glm::vec4 outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
  glm::vec4 fill_color_{0.0F, 0.0F, 0.0F, 1.0F};
};
#endif  // SHAPES_HPP
