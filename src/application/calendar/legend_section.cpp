#include "legend_section.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "../../domain/shape_configuration.hpp"
#include "../../infrastructure/graphics/child_pool.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "calendar_scene_nodes.hpp"
#include "section_context.hpp"

namespace calendar_sections {

void BuildLegend(const SectionContext& ctx) {
  ShapeChildPool<BoxesShape> entry_pool(
      ctx.nodes.legend_entries, ctx.rectangles_shader, calendar_layers::kBars);
  auto entry_labels = detail::TextPool(ctx, ctx.nodes.legend_labels);

  const size_t number_entry_frames =
      (ctx.date_categories.Items().size() + 1) * 2;
  std::vector<RectF> legend_entries_frames(number_entry_frames);
  const auto entries_width =
      ctx.layout.LegendArea().Width() / static_cast<float>(number_entry_frames);

  for (size_t index = 0; index < number_entry_frames; ++index) {
    const auto float_index = static_cast<float>(index);
    const auto left =
        ctx.layout.LegendArea().Left() + (entries_width * float_index);
    legend_entries_frames.at(index) = ctx.layout.LegendArea();
    legend_entries_frames.at(index).SetLeft(left);
    legend_entries_frames.at(index).SetRight(left + entries_width);
  }

  std::vector<RectF> bar_cells;

  auto print_strings = ctx.date_categories.GetDateCategoryNames();
  print_strings.emplace_back(ShapeConfigSet::FixedConfigurationLabel(
      ShapeConfigSet::kAnnualCoverageKey));

  std::string string_max_length;
  for (const auto& current_string : print_strings) {
    if (current_string.length() > string_max_length.length()) {
      string_max_length = current_string;
    }
  }

  const auto legend_font_size = ctx.font->AdjustTextSize(
      legend_entries_frames.at(0), string_max_length,
      Font::TextScale{.height_ratio = detail::kFontScaleMin,
                      .width_ratio = detail::kFontScaleMax});

  const std::size_t year_count = ctx.calendar_config.YearCount();
  for (size_t index = 0; index < ctx.date_categories.Items().size(); ++index) {
    const auto label_index = index * 2;
    detail::SetCenteredText(
        ctx, entry_labels, std::string("legend label ") + std::to_string(index),
        ctx.date_categories.Items().at(index).GetName(),
        legend_entries_frames.at(label_index).Center(), legend_font_size);

    if (year_count > 0U) {
      const auto current_height = ctx.layout.GetSubArea(0, 1).Height();
      auto current_cell = legend_entries_frames.at(label_index + 1);
      const auto current_vertical_center = current_cell.Center()[1];
      current_cell.SetBottom(current_vertical_center -
                             (current_height * detail::kHalf));
      current_cell.SetTop(current_vertical_center +
                          (current_height * detail::kHalf));
      bar_cells.emplace_back(current_cell);

      auto current_shape_config =
          ctx.shape_config.GetDynamicConfiguration(index);

      const auto entry =
          entry_pool.Next(std::string("legend bar ") + std::to_string(index));
      entry.node->SetStyleId(current_shape_config.Key());
      entry.shape.SetShape(current_cell, current_shape_config.LineWidth());
      entry.shape.SetColors(current_shape_config.OutlineColor(),
                            current_shape_config.FillColor());
    }
  }

  {
    detail::SetCenteredText(
        ctx, entry_labels, std::string("legend label annual coverage"),
        std::string(ShapeConfigSet::FixedConfigurationLabel(
            ShapeConfigSet::kAnnualCoverageKey)),
        legend_entries_frames.at(legend_entries_frames.size() - 2).Center(),
        legend_font_size);

    if (year_count > 0U) {
      const auto current_height = ctx.layout.GetSubArea(0, 0).Height();
      auto current_cell =
          legend_entries_frames.at(legend_entries_frames.size() - 1);
      const auto current_vertical_center = current_cell.Center()[1];
      current_cell.SetBottom(current_vertical_center -
                             (current_height * detail::kHalf));
      current_cell.SetTop(current_vertical_center +
                          (current_height * detail::kHalf));
      bar_cells.emplace_back(current_cell);

      auto current_shape_config = ctx.shape_config.GetShapeConfiguration(
          std::string(ShapeConfigSet::kAnnualCoverageKey));

      const auto entry =
          entry_pool.Next(std::string("legend bar annual coverage"));
      entry.node->SetStyleId(current_shape_config.Key());
      entry.shape.SetShape(current_cell, current_shape_config.LineWidth());
      entry.shape.SetColors(current_shape_config.OutlineColor(),
                            current_shape_config.FillColor());
    }
  }
}
}  // namespace calendar_sections
