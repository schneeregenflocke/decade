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

// Creates a centered text node named `name` under `parent`: a FontShape on the
// given draw layer rendering `text` centered at `center` with font height
// `size`. Concentrates the "make FontShape, SetFont, centre, attach on a layer"
// sequence used by every label group.
inline void AddCenteredText(const std::shared_ptr<SceneNode>& parent,
                            const std::string& name, const std::string& text,
                            const glm::vec3& center, float size,
                            Shader& font_shader,
                            const std::shared_ptr<Font>& font, int draw_layer) {
  auto shape = std::make_unique<FontShape>(font_shader);
  shape->SetFont(font);
  shape->SetShapeCentered(text, center, size);
  auto node = std::make_shared<SceneNode>(name, std::move(shape));
  node->SetDrawLayer(draw_layer);
  parent->AddChild(node);
}

}  // namespace scene_shapes

#endif  // SCENE_SHAPE_FILLER_HPP
