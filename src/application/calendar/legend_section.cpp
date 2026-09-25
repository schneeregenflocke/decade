#include "legend_section.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "../../domain/shape_configuration.hpp"
#include "../../infrastructure/graphics/child_pool.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_shape_filler.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "calendar_scene_nodes.hpp"
#include "section_context.hpp"

namespace calendar_sections {

namespace {

// One legend entry: a label in one frame and, beside it, a bar styled like the
// ones it explains. `id` names both nodes.
struct LegendEntry {
  std::string id;
  std::string label;
  RectF label_frame;
  RectF bar_frame;
  float bar_height{0.0F};
  ShapeConfiguration style;
};

void AddLegendEntry(const SectionContext& ctx,
                    scene_shapes::TextChildPool& labels,
                    ShapeChildPool<BoxesShape>& bars, const LegendEntry& entry,
                    float font_size) {
  detail::SetCenteredText(ctx, labels, "legend label " + entry.id, entry.label,
                          entry.label_frame.Center(), font_size);

  RectF cell = entry.bar_frame;
  const auto vertical_center = cell.Center()[1];
  cell.SetBottom(vertical_center - (entry.bar_height * detail::kHalf));
  cell.SetTop(vertical_center + (entry.bar_height * detail::kHalf));

  const auto bar = bars.Next("legend bar " + entry.id);
  bar.node->SetStyleId(entry.style.Key());
  bar.shape.SetShape(cell, entry.style.LineWidth());
  bar.shape.SetColors(entry.style.OutlineColor(), entry.style.FillColor());
}

// The legend area split into equal frames, two per entry: label, then bar.
std::vector<RectF> LegendFrames(const SectionContext& ctx,
                                std::size_t entry_count) {
  const std::size_t frame_count = entry_count * 2;
  const RectF area = ctx.layout.LegendArea();
  const auto frame_width = area.Width() / static_cast<float>(frame_count);
  std::vector<RectF> frames(frame_count, area);
  for (std::size_t index = 0; index < frame_count; ++index) {
    const auto left = area.Left() + (frame_width * static_cast<float>(index));
    frames[index].SetLeft(left);
    frames[index].SetRight(left + frame_width);
  }
  return frames;
}

}  // namespace

void BuildLegend(const SectionContext& ctx) {
  ShapeChildPool<BoxesShape> bars(
      ctx.nodes.legend_entries, ctx.rectangles_shader, calendar_layers::kBars);
  auto labels = detail::TextPool(ctx, ctx.nodes.legend_labels);

  const auto& categories = ctx.date_categories.Items();
  const std::string coverage_label(ShapeConfigSet::FixedConfigurationLabel(
      ShapeConfigSet::kAnnualCoverageKey));
  const std::vector<RectF> frames = LegendFrames(ctx, categories.size() + 1);

  // Every label shares one font size: the one the longest label fits at.
  std::string longest_label;
  for (const auto& category : categories) {
    if (category.GetName().length() > longest_label.length()) {
      longest_label = category.GetName();
    }
  }
  if (coverage_label.length() > longest_label.length()) {
    longest_label = coverage_label;
  }
  const auto font_size = ctx.font->AdjustTextSize(
      frames.front(), longest_label,
      Font::TextScale{.height_ratio = detail::kFontScaleMin,
                      .width_ratio = detail::kFontScaleMax});

  const float category_bar_height = ctx.layout.GetSubArea(0, 1).Height();
  for (std::size_t index = 0; index < categories.size(); ++index) {
    AddLegendEntry(ctx, labels, bars,
                   {.id = std::to_string(index),
                    .label = categories[index].GetName(),
                    .label_frame = frames[index * 2],
                    .bar_frame = frames[(index * 2) + 1],
                    .bar_height = category_bar_height,
                    .style = ctx.shape_config.GetDynamicConfiguration(index)},
                   font_size);
  }

  AddLegendEntry(ctx, labels, bars,
                 {.id = "annual coverage",
                  .label = coverage_label,
                  .label_frame = frames[frames.size() - 2],
                  .bar_frame = frames.back(),
                  .bar_height = ctx.layout.GetSubArea(0, 0).Height(),
                  .style = ctx.shape_config.GetShapeConfiguration(
                      ShapeConfigSet::kAnnualCoverageKey)},
                 font_size);
}
}  // namespace calendar_sections
