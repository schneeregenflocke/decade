#ifndef BAR_SECTIONS_HPP
#define BAR_SECTIONS_HPP

#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../../domain/date.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "section_context.hpp"

// The date bars with their labels and pick boxes, plus the per-year totals
// beside them — both read the same bar data, hence one file.

namespace calendar_sections {

[[nodiscard]] inline BarSceneResult BuildBars(const SectionContext& ctx) {
  BarSceneResult result;

  const auto& node = ctx.nodes.date_bars;
  node->RemoveChildren();

  const auto number_groups = ctx.date_groups.Items().size();
  std::vector<std::shared_ptr<SceneNode>> group_nodes;
  group_nodes.reserve(number_groups);
  for (size_t index = 0; index < number_groups; ++index) {
    auto group_node = std::make_shared<SceneNode>(std::string("group node ") +
                                                  std::to_string(index));
    node->AddChild(group_node);
    group_nodes.push_back(group_node);
  }

  const auto& node_labels = ctx.nodes.date_bar_labels;
  node_labels->RemoveChildren();

  const TimelineProjection projection(ctx.calendar_config);
  const auto number_bars = ctx.date_entry_bars.GetNumberBars();
  for (size_t index = 0; index < number_bars; ++index) {
    const auto& bar = ctx.date_entry_bars.GetBar(index);
    if (!ctx.calendar_config.IsInSpan(bar.GetYear())) {
      continue;
    }
    const auto current_group = static_cast<size_t>(bar.GetGroup());
    auto current_shape_config =
        ctx.shape_config.GetDynamicConfiguration(current_group);

    const auto row = projection.RowForYear(bar.GetYear());
    const auto current_sub_cell = ctx.layout.GetSubFrame(row, 1);

    const auto bar_left =
        current_sub_cell.Left() + (bar.GetFirstDay() * ctx.layout.DayWidth());
    const auto bar_width =
        (bar.GetLastDay() - bar.GetFirstDay()) * ctx.layout.DayWidth();
    const auto bar_height = current_sub_cell.Height();

    // Each bar is its own node: the position lives in the node transform (ready
    // for dragging/animating), the size lives in the shape geometry. A pure
    // translation keeps the outline width constant, which a scale matrix would
    // distort. The bar's world rect is therefore unchanged.
    auto bar_node = std::make_shared<SceneNode>(std::string("bar ") +
                                                std::to_string(index));
    bar_node->SetModelMatrix(glm::translate(
        glm::mat4(1.0F),
        glm::vec3(bar_left, current_sub_cell.Bottom(), detail::kZero)));
    bar_node->SetStyleId(current_shape_config.Name());

    // Page-space box for hit-testing. The node's world position is
    // layout.PrintAreaOrigin() + (bar_left, sub_cell.Bottom()), so the
    // page-space rect is the local bar rect shifted by that origin.
    const PickId pick_id{.kind = PickId::Kind::kBar, .index = index};
    result.pick_boxes.push_back(PickBox{
        .id = pick_id,
        .rect =
            rectf(bar_left + ctx.layout.PrintAreaOrigin().x,
                  bar_left + bar_width + ctx.layout.PrintAreaOrigin().x,
                  current_sub_cell.Bottom() + ctx.layout.PrintAreaOrigin().y,
                  current_sub_cell.Bottom() + bar_height +
                      ctx.layout.PrintAreaOrigin().y)});

    auto bar_shape = std::make_unique<RectanglesShape>(ctx.rectangles_shader);
    bar_shape->SetShape(
        rectf(detail::kZero, bar_width, detail::kZero, bar_height),
        current_shape_config.LineWidth());
    bar_shape->SetColors(current_shape_config.OutlineColor(),
                         current_shape_config.FillColor());
    bar_node->SetShape(std::move(bar_shape));
    bar_node->SetDrawLayer(calendar_layers::kBars);
    group_nodes.at(current_group)->AddChild(bar_node);
    result.bar_nodes.emplace(index, bar_node);

    auto current_text_cell = ctx.layout.GetSubFrame(row, 2);
    current_text_cell.SetLeft(bar_left);
    current_text_cell.SetRight(bar_left + bar_width);

    detail::AddCenteredText(
        ctx, node_labels, std::string("label node ") + std::to_string(index),
        bar.GetText(), current_text_cell.Center(), current_text_cell.Height());
  }

  return result;
}

inline void BuildYearTotals(const SectionContext& ctx) {
  const auto& node_cells = ctx.nodes.year_totals;
  const auto& node_text = ctx.nodes.year_total_labels;
  node_text->RemoveChildren();

  const std::size_t span_years = ctx.date_entry_bars.GetSpan();
  if (span_years == 0) {
    return;
  }

  const TimelineProjection projection(ctx.calendar_config);
  std::vector<rectf> year_totals_cells(span_years);

  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year =
        ctx.date_entry_bars.GetFirstYear() + static_cast<int>(index);
    if (ctx.calendar_config.IsInSpan(current_year)) {
      const auto row = projection.RowForYear(current_year);
      const auto current_cell = ctx.layout.GetSubFrame(row, 0);

      rectf year_total_cell = current_cell;
      const auto year_total_width =
          static_cast<float>(ctx.date_entry_bars.GetAnnualTotal(index)) *
          ctx.layout.DayWidth();
      year_total_cell.SetRight(current_cell.Left() + year_total_width);
      year_totals_cells.at(index) = year_total_cell;

      const auto number_days = DaysInYear(current_year);

      const float percent =
          static_cast<float>(ctx.date_entry_bars.GetAnnualTotal(index)) /
          static_cast<float>(number_days);

      std::ostringstream year_total_stream;
      year_total_stream << std::fixed << std::setprecision(1)
                        << percent * detail::kPercentScale << " %";
      const auto year_total_text = year_total_stream.str();
      const auto year_total_text_width =
          ctx.font->TextWidth(year_total_text, year_total_cell.Height());

      rectf year_total_text_cell;
      year_total_text_cell.SetLeft(year_total_cell.Right() +
                                   current_cell.Height());
      year_total_text_cell.SetRight(year_total_text_cell.Left() +
                                    year_total_text_width);
      year_total_text_cell.SetBottom(year_total_cell.Bottom());
      year_total_text_cell.SetTop(year_total_cell.Top());

      detail::AddCenteredText(
          ctx, node_text,
          std::string("year total label ") + std::to_string(index),
          year_total_text, year_total_text_cell.Center(),
          year_total_text_cell.Height());
    }
  }

  detail::FillRectangles(
      node_cells, year_totals_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kYearsTotals));
}

}  // namespace calendar_sections

#endif  // BAR_SECTIONS_HPP
