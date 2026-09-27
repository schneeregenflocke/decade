#include "scene_shape_filler.hpp"

#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <string>

#include "font.hpp"

namespace scene_shapes {

void SetCenteredText(FontShape& shape, const std::string& text,
                     const glm::vec3& center, float size,
                     const std::shared_ptr<Font>& font) {
  shape.SetFont(font);
  shape.SetShapeCentered(text, center, size);
}

}  // namespace scene_shapes
