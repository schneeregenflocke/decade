#include "section_context.hpp"

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <string>

#include "../../domain/calendar_config.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_period.hpp"
#include "../../infrastructure/graphics/rect.hpp"
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

RectF PeriodArea(const SectionContext& ctx, const DatePeriod& period,
                 BandPart part) {
  const std::size_t row = ctx.projection.RowOf(period.Begin());
  const Date row_begin = ctx.projection.RowPeriod(row).Begin();
  const auto offset = [&](const Date& date) {
    return static_cast<float>(Date::DaysBetween(row_begin, date)) *
           ctx.layout.DayWidth();
  };
  RectF area = ctx.layout.GetPartArea(row, part);
  const float row_left = area.Left();
  area.SetLeft(row_left + offset(period.Begin()));
  area.SetRight(row_left + offset(period.End()));
  return area;
}

}  // namespace calendar_sections::detail
