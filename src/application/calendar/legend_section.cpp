#include "legend_section.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "../../domain/band_part.hpp"
#include "../../domain/calendar_config.hpp"
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

// One legend entry: a label and, beside it, a bar styled like the ones it
// explains. `id` names both nodes.
struct LegendEntry {
  std::string id;
  std::string label;
  float bar_height{0.0F};
  ShapeConfiguration style;
};

// Where one entry goes: a frame for its label, then one for its bar.
struct LegendSlot {
  RectF label_frame;
  RectF bar_frame;
};

void AddLegendEntry(const SectionContext& ctx,
                    scene_shapes::TextChildPool& labels,
                    ShapeChildPool<BoxesShape>& bars, const LegendEntry& entry,
                    const LegendSlot& slot, float font_size) {
  detail::SetCenteredText(ctx, labels, "legend label " + entry.id, entry.label,
                          slot.label_frame.Center(), font_size);

  RectF cell = slot.bar_frame;
  const auto vertical_center = cell.Center()[1];
  cell.SetBottom(vertical_center - (entry.bar_height * detail::kHalf));
  cell.SetTop(vertical_center + (entry.bar_height * detail::kHalf));

  const auto bar = bars.Next("legend bar " + entry.id);
  bar.node->SetStyleId(entry.style.Key());
  bar.shape.SetShape(cell, entry.style.LineWidth());
  bar.shape.SetColors(entry.style.OutlineColor(), entry.style.FillColor());
}

// One slot per entry from the legend area's left edge, each half label, half
// bar.
std::vector<LegendSlot> LegendSlots(const SectionContext& ctx,
                                    std::size_t entry_count) {
  const RectF area = ctx.layout.LegendArea();
  const auto frame_width =
      ctx.layout.LegendEntryWidth(entry_count) * detail::kHalf;
  const auto frame_at = [&](std::size_t frame_index) {
    RectF frame = area;
    frame.SetLeft(area.Left() +
                  (frame_width * static_cast<float>(frame_index)));
    frame.SetRight(frame.Left() + frame_width);
    return frame;
  };
  std::vector<LegendSlot> slots;
  slots.reserve(entry_count);
  for (std::size_t index = 0; index < entry_count; ++index) {
    slots.push_back({.label_frame = frame_at(index * 2),
                     .bar_frame = frame_at((index * 2) + 1)});
  }
  return slots;
}

}  // namespace

LegendResult BuildLegend(const SectionContext& ctx) {
  ShapeChildPool<BoxesShape> bars(
      ctx.nodes.legend_entries, ctx.rectangles_shader, calendar_layers::kBars);
  auto labels = detail::TextPool(ctx, ctx.nodes.legend_labels);

  const auto& categories = ctx.date_categories.Items();
  std::vector<LegendEntry> entries;
  entries.reserve(categories.size() + 1);
  const float category_bar_height = ctx.layout.BandPartHeight(BandPart::kDays);
  for (std::size_t index = 0; index < categories.size(); ++index) {
    entries.push_back(
        {.id = std::to_string(index),
         .label = categories[index].GetName(),
         .bar_height = category_bar_height,
         .style = ctx.shape_config.GetDynamicConfiguration(index)});
  }
  if (ctx.calendar_config.ShowsAnnualCoverage()) {
    entries.push_back(
        {.id = "annual coverage",
         .label = std::string(ShapeConfigSet::FixedConfigurationLabel(
             ShapeConfigSet::kAnnualCoverageKey)),
         .bar_height = ctx.layout.BandPartHeight(BandPart::kCoverage),
         .style = ctx.shape_config.GetShapeConfiguration(
             ShapeConfigSet::kAnnualCoverageKey)});
  }
  RectF drawn = ctx.layout.LegendArea();
  if (entries.empty()) {
    drawn.SetRight(drawn.Left());
    return {.area = drawn, .entry_width = ctx.layout.LegendEntryWidth(1)};
  }

  const std::vector<LegendSlot> slots = LegendSlots(ctx, entries.size());

  // Every label shares one font size: the one the longest label fits at.
  const auto longest = std::ranges::max_element(
      entries, {},
      [](const LegendEntry& entry) { return entry.label.length(); });
  const auto font_size = ctx.font->AdjustTextSize(
      slots.front().label_frame, longest->label,
      Font::TextScale{.height_ratio = detail::kFontScaleMin,
                      .width_ratio = detail::kFontScaleMax});

  for (std::size_t index = 0; index < entries.size(); ++index) {
    AddLegendEntry(ctx, labels, bars, entries[index], slots[index], font_size);
  }
  drawn.SetRight(slots.back().bar_frame.Right());
  return {.area = drawn,
          .entry_width = ctx.layout.LegendEntryWidth(entries.size())};
}
}  // namespace calendar_sections
