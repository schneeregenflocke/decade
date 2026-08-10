#include "scene_shape_filler.hpp"

#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <string>

#include "child_pool.hpp"
#include "font.hpp"

namespace scene_shapes {

void SetCenteredText(TextChildPool& pool, const std::string& name,
                     const std::string& text, const glm::vec3& center,
                     float size, const std::shared_ptr<Font>& font) {
  FontShape& shape = pool.Next(name).shape;
  shape.SetFont(font);
  shape.SetShapeCentered(text, center, size);
}

}  // namespace scene_shapes
