#ifndef LEGEND_SECTION_HPP
#define LEGEND_SECTION_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../../domain/shape_configuration.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "section_context.hpp"

// The legend below the calendar: one label plus one sample bar per date group,
// and the annual sum after them.

namespace calendar_sections {

inline void BuildLegend(const SectionContext& ctx) {
  const auto& node_entries = ctx.nodes.legend_entries;
  node_entries->RemoveChildren();

  const auto& node_text = ctx.nodes.legend_labels;
  node_text->RemoveChildren();

  const size_t number_entry_frames = (ctx.date_groups.Items().size() + 1) * 2;
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

  auto print_strings = ctx.date_groups.GetDateGroupsNames();
  print_strings.emplace_back("Annual Sums");

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

  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  for (size_t index = 0; index < ctx.date_groups.Items().size(); ++index) {
    const auto label_index = index * 2;
    detail::AddCenteredText(
        ctx, node_text, std::string("legend label ") + std::to_string(index),
        ctx.date_groups.Items().at(index).GetName(),
        legend_entries_frames.at(label_index).Center(), legend_font_size);

    if (span_years > 0U) {
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

      auto node_entry = std::make_shared<SceneNode>(std::string("legend bar ") +
                                                    std::to_string(index));
      node_entry->SetDrawLayer(calendar_layers::kBars);
      node_entry->SetStyleId(current_shape_config.Name());
      node_entries->AddChild(node_entry);

      auto entry_shape = std::make_unique<BoxesShape>(ctx.rectangles_shader);
      entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
      entry_shape->SetColors(current_shape_config.OutlineColor(),
                             current_shape_config.FillColor());
      node_entry->SetShape(std::move(entry_shape));
    }
  }

  {
    detail::AddCenteredText(
        ctx, node_text, std::string("legend label year total"), "Annual sum",
        legend_entries_frames.at(legend_entries_frames.size() - 2).Center(),
        legend_font_size);

    if (span_years > 0U) {
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
          std::string(ShapeConfigSet::kYearsTotals));

      auto node_entry =
          std::make_shared<SceneNode>(std::string("legend bar annual sum"));
      node_entry->SetDrawLayer(calendar_layers::kBars);
      node_entry->SetStyleId(current_shape_config.Name());
      node_entries->AddChild(node_entry);

      auto entry_shape = std::make_unique<BoxesShape>(ctx.rectangles_shader);
      entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
      entry_shape->SetColors(current_shape_config.OutlineColor(),
                             current_shape_config.FillColor());
      node_entry->SetShape(std::move(entry_shape));
    }
  }
}

}  // namespace calendar_sections

#endif  // LEGEND_SECTION_HPP
