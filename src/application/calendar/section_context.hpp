#ifndef SECTION_CONTEXT_HPP
#define SECTION_CONTEXT_HPP

#include <cstddef>
#include <glm/vec3.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../domain/calendar_config.hpp"
#include "../../domain/date_entry_bars.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/font_config.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/title_config.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/scene_shape_filler.hpp"
#include "../../infrastructure/graphics/shaders.hpp"
#include "../../infrastructure/graphics/shape_node.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "calendar_layout.hpp"
#include "calendar_scene_nodes.hpp"

// The shared ground of the section builders: what every one of them gets
// handed, what BuildBars hands back, and the two adapters that map a domain
// ShapeConfiguration onto the general scene primitives.

namespace calendar_sections {

struct SectionContext {
  const CalendarSceneNodes& nodes;
  const CalendarLayout& layout;
  const ShapeConfigSet& shape_config;
  const CalendarConfig& calendar_config;
  const TitleConfig& title_config;
  const DateGroups& date_groups;
  const DateEntryBars& date_entry_bars;
  // Empty while nobody edits text in the canvas.
  const std::optional<TextEditView>& text_edit;
  const std::shared_ptr<Font>& font;
  // The application-wide chosen font including its point size.
  const FontConfig& font_config;
  Shader& rectangles_shader;
  Shader& font_shader;
};

// Output of BuildBars consumed by the coordinator: the page-space pick boxes
// for the picking layer and the per-index bar nodes for the hover highlight.
struct BarSceneResult {
  std::vector<PickBox> pick_boxes;
  std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes;
};

namespace detail {

inline constexpr float kZero = 0.0F;
inline constexpr float kHalf = 0.5F;
inline constexpr float kFontScaleMin = 0.5F;
inline constexpr float kFontScaleMax = 0.75F;
inline constexpr float kPercentScale = 100.0F;
inline constexpr std::size_t kMonthNameBufferSize = 100;

// A calendar-side adapter over scene_shapes::FillRectangles: it maps a domain
// ShapeConfiguration onto the general primitives and notes the style ID at
// which the scene tree shows the values of the configuration.
template <typename Shapes>
inline void FillRectangles(const ShapeNode<BoxesShape>& node,
                           const Shapes& shapes,
                           const ShapeConfiguration& config) {
  scene_shapes::FillRectangles(node, shapes, config.OutlineColor(),
                               config.FillColor(), config.LineWidth());
  node.Node()->SetStyleId(config.Name());
}

// Adapter over scene_shapes::AddCenteredText supplying the text draw layer.
inline void AddCenteredText(const SectionContext& ctx,
                            const std::shared_ptr<SceneNode>& parent,
                            const std::string& name, const std::string& text,
                            const glm::vec3& center, float size) {
  scene_shapes::AddCenteredText(parent, name, text, center, size,
                                ctx.font_shader, ctx.font,
                                calendar_layers::kText);
}

}  // namespace detail

}  // namespace calendar_sections

#endif  // SECTION_CONTEXT_HPP
