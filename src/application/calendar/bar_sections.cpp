#include "bar_sections.hpp"

#include <cstddef>
#include <deque>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include "../../domain/date.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../infrastructure/graphics/child_pool.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "calendar_scene_nodes.hpp"
#include "section_context.hpp"

namespace calendar_sections {

BarSceneResult BuildBars(const SectionContext& ctx) {
  BarSceneResult result;

  ChildPool category_pool(ctx.nodes.date_bars);
  const auto number_categories = ctx.date_categories.Items().size();
  // A deque, not a vector: the pool is deliberately neither copyable nor
  // movable, and a deque places its elements without ever moving them.
  std::deque<ShapeChildPool<BoxesShape>> bar_pools;
  for (size_t index = 0; index < number_categories; ++index) {
    bar_pools.emplace_back(category_pool.Next(std::string("category node ") +
                                              std::to_string(index)),
                           ctx.rectangles_shader, calendar_layers::kBars);
  }

  auto bar_labels = detail::TextPool(ctx, ctx.nodes.date_bar_labels);

  const TimelineProjection projection(ctx.calendar_config);
  const auto number_bars = ctx.date_entry_bars.GetNumberBars();
  for (size_t index = 0; index < number_bars; ++index) {
    const auto& bar_data = ctx.date_entry_bars.GetBar(index);
    if (!ctx.calendar_config.ShowsYear(bar_data.GetYear())) {
      continue;
    }
    const auto current_category = static_cast<size_t>(bar_data.GetCategory());
    auto current_shape_config =
        ctx.shape_config.GetDynamicConfiguration(current_category);

    const auto row = projection.RowForYear(bar_data.GetYear());
    const auto current_sub_cell = ctx.layout.GetSubArea(row, 1);

    const auto bar_left = current_sub_cell.Left() +
                          (bar_data.GetFirstDay() * ctx.layout.DayWidth());
    const auto bar_width = (bar_data.GetLastDay() - bar_data.GetFirstDay()) *
                           ctx.layout.DayWidth();
    const auto bar_height = current_sub_cell.Height();

    // Each bar is its own node: the position lives in the node transform (ready
    // for dragging/animating), the size lives in the shape geometry. A pure
    // translation keeps the outline width constant, which a scale matrix would
    // distort. The bar's world rect is therefore unchanged.
    const auto bar = bar_pools.at(current_category)
                         .Next(std::string("bar ") + std::to_string(index));
    bar.node->SetModelMatrix(glm::translate(
        glm::mat4(1.0F),
        glm::vec3(bar_left, current_sub_cell.Bottom(), detail::kZero)));
    bar.node->SetStyleId(current_shape_config.Key());

    // Page-space box for hit-testing. The node's world position is
    // layout.PrintAreaOrigin() + (bar_left, sub_cell.Bottom()), so the
    // page-space rect is the local bar rect shifted by that origin.
    const PickId pick_id{.kind = PickId::Kind::kBar, .index = index};
    result.pick_boxes.push_back(PickBox{
        .id = pick_id,
        .rect =
            RectF(bar_left + ctx.layout.PrintAreaOrigin().x,
                  bar_left + bar_width + ctx.layout.PrintAreaOrigin().x,
                  current_sub_cell.Bottom() + ctx.layout.PrintAreaOrigin().y,
                  current_sub_cell.Bottom() + bar_height +
                      ctx.layout.PrintAreaOrigin().y)});

    bar.shape.SetShape(
        RectF(detail::kZero, bar_width, detail::kZero, bar_height),
        current_shape_config.LineWidth());
    bar.shape.SetColors(current_shape_config.OutlineColor(),
                        current_shape_config.FillColor());
    result.bar_nodes.emplace(index, bar.node);

    auto current_text_cell = ctx.layout.GetSubArea(row, 2);
    current_text_cell.SetLeft(bar_left);
    current_text_cell.SetRight(bar_left + bar_width);

    detail::SetCenteredText(ctx, bar_labels,
                            std::string("label node ") + std::to_string(index),
                            bar_data.GetText(), current_text_cell.Center(),
                            current_text_cell.Height());
  }

  return result;
}

void BuildAnnualCoverage(const SectionContext& ctx) {
  const auto& node_cells = ctx.nodes.annual_coverage;
  auto coverage_labels =
      detail::TextPool(ctx, ctx.nodes.annual_coverage_labels);

  const std::size_t drawn_years = ctx.calendar_config.ShowsAnnualCoverage()
                                      ? ctx.date_entry_bars.GetSpan()
                                      : 0;
  const TimelineProjection projection(ctx.calendar_config);
  std::vector<RectF> coverage_cells(drawn_years);

  for (std::size_t index = 0; index < drawn_years; ++index) {
    const int current_year =
        ctx.date_entry_bars.GetFirstYear() + static_cast<int>(index);
    if (ctx.calendar_config.ShowsYear(current_year)) {
      const auto row = projection.RowForYear(current_year);
      const auto current_cell = ctx.layout.GetSubArea(row, 0);

      RectF coverage_cell = current_cell;
      const auto coverage_width =
          static_cast<float>(ctx.date_entry_bars.GetCoveredDays(index)) *
          ctx.layout.DayWidth();
      coverage_cell.SetRight(current_cell.Left() + coverage_width);
      coverage_cells.at(index) = coverage_cell;

      const auto number_days = DaysInYear(current_year);

      const float percent =
          static_cast<float>(ctx.date_entry_bars.GetCoveredDays(index)) /
          static_cast<float>(number_days);

      std::ostringstream coverage_stream;
      coverage_stream << std::fixed << std::setprecision(1)
                      << percent * detail::kPercentScale << " %";
      const auto coverage_text = coverage_stream.str();
      const auto coverage_text_width =
          ctx.font->TextWidth(coverage_text, coverage_cell.Height());

      RectF coverage_text_cell;
      coverage_text_cell.SetLeft(coverage_cell.Right() + current_cell.Height());
      coverage_text_cell.SetRight(coverage_text_cell.Left() +
                                  coverage_text_width);
      coverage_text_cell.SetBottom(coverage_cell.Bottom());
      coverage_text_cell.SetTop(coverage_cell.Top());

      detail::SetCenteredText(
          ctx, coverage_labels,
          std::string("annual coverage label ") + std::to_string(index),
          coverage_text, coverage_text_cell.Center(),
          coverage_text_cell.Height());
    }
  }

  detail::FillRectangles(node_cells, coverage_cells,
                         ctx.shape_config.GetShapeConfiguration(
                             ShapeConfigSet::kAnnualCoverageKey));
}

}  // namespace calendar_sections
