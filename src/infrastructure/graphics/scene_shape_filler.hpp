#ifndef SCENE_SHAPE_FILLER_HPP
#define SCENE_SHAPE_FILLER_HPP

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>

#include "font.hpp"
#include "scene_graph.hpp"
#include "shaders.hpp"
#include "shape_node.hpp"
#include "shapes.hpp"

// Infrastructure: generic, domain-free helpers for the two scene-node
// construction shapes that recur across the calendar's section builders —
// filling a BoxesShape node and attaching a centered text node. They take
// plain colours/line width and a shader/font (never a domain
// ShapeConfiguration) so this stays a reusable graphics utility; the
// calendar-specific mapping from a configuration to these primitives lives in
// the calendar adapter (src/application/calendar/).
namespace scene_shapes {

// Fills the BoxesShape carried by `node` with the given rectangle(s) (a
// single RectF or a vector of them — BoxesShape::SetShape is overloaded) and
// the outline/fill colours. The handle carries the shape type, so nothing has
// to be asked of the RTTI here.
template <typename Shapes>
inline void FillRectangles(const ShapeNode<BoxesShape>& node,
                           const Shapes& shapes, const glm::vec4& outline_color,
                           const glm::vec4& fill_color, float line_width) {
  BoxesShape& shape = node.Shape();
  shape.SetShape(shapes, line_width);
  shape.SetColors(outline_color, fill_color);
}

// A pool of centred text children. Sets the next child to `text`, centred at
// `center` with font height `size`, and keeps the node it already had —
// including its GL buffers. The label groups all share this shape.
using TextChildPool = ShapeChildPool<FontShape>;

inline void SetCenteredText(TextChildPool& pool, const std::string& name,
                            const std::string& text, const glm::vec3& center,
                            float size, const std::shared_ptr<Font>& font) {
  FontShape& shape = pool.Next(name).shape;
  shape.SetFont(font);
  shape.SetShapeCentered(text, center, size);
}

}  // namespace scene_shapes

#endif  // SCENE_SHAPE_FILLER_HPP
