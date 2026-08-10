#include "section_context.hpp"

#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <string>

#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/scene_shape_filler.hpp"
#include "calendar_scene_nodes.hpp"

namespace calendar_sections::detail {

scene_shapes::TextChildPool TextPool(const SectionContext& ctx,
                                     const std::shared_ptr<SceneNode>& parent) {
  return {parent, ctx.font_shader, calendar_layers::kText};
}

void SetCenteredText(const SectionContext& ctx,
                     scene_shapes::TextChildPool& pool, const std::string& name,
                     const std::string& text, const glm::vec3& center,
                     float size) {
  scene_shapes::SetCenteredText(pool, name, text, center, size, ctx.font);
}

}  // namespace calendar_sections::detail
