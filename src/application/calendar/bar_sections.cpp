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
#include "../../domain/date_period.hpp"
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

  // One bar per row an entry reaches; the index counts the bars drawn.
  std::size_t index = 0;
  for (size_t entry = 0; entry < ctx.date_entry_bars.GetNumberBars(); ++entry) {
    const auto& bar_data = ctx.date_entry_bars.GetBar(entry);
    const auto current_category = static_cast<size_t>(bar_data.GetCategory());
    auto current_shape_config =
        ctx.shape_config.GetDynamicConfiguration(current_category);

    for (const DatePeriod& piece :
         ctx.projection.SplitAtRowBoundaries(bar_data.Period())) {
      const RectF bar_area = detail::PeriodArea(ctx, piece, 1);

      // Each bar is its own node: the position lives in the node transform
      // (ready for dragging/animating), the size lives in the shape geometry.
      // A pure translation keeps the outline width constant, which a scale
      // matrix would distort. The bar's world rect is therefore unchanged.
      const auto bar = bar_pools.at(current_category)
                           .Next(std::string("bar ") + std::to_string(index));
      bar.node->SetModelMatrix(glm::translate(
          glm::mat4(1.0F),
          glm::vec3(bar_area.Left(), bar_area.Bottom(), detail::kZero)));
      bar.node->SetStyleId(current_shape_config.Key());

      // Page-space box for hit-testing: the node's world position is
      // layout.PrintAreaOrigin() + the bar's corner.
      const PickId pick_id{.kind = PickId::Kind::kBar, .index = index};
      result.pick_boxes.push_back(
          PickBox{.id = pick_id,
                  .rect = bar_area.Shift(ctx.layout.PrintAreaOrigin().x,
                                         ctx.layout.PrintAreaOrigin().y)});

      bar.shape.SetShape(RectF(detail::kZero, bar_area.Width(), detail::kZero,
                               bar_area.Height()),
                         current_shape_config.LineWidth());
      bar.shape.SetColors(current_shape_config.OutlineColor(),
                          current_shape_config.FillColor());
      result.bar_nodes.emplace(index, bar.node);

      detail::SetCenteredText(
          ctx, bar_labels, std::string("label node ") + std::to_string(index),
          bar_data.GetText(), detail::PeriodArea(ctx, piece, 2).Center(),
          ctx.layout.BandSubHeight(2));
      ++index;
    }
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
  std::vector<RectF> coverage_cells;

  for (std::size_t index = 0; index < drawn_years; ++index) {
    const int current_year =
        ctx.date_entry_bars.GetFirstYear() + static_cast<int>(index);
    if (!ctx.calendar_config.ShowsYear(current_year)) {
      continue;
    }
    const DatePeriod year(Date::FromYmd(current_year, 1, 1),
                          Date::FromYmd(current_year + 1, 1, 1));
    const RectF year_cell = detail::PeriodArea(ctx, year, 0);
    const auto covered_days = ctx.date_entry_bars.GetCoveredDays(index);

    RectF coverage_cell = year_cell;
    coverage_cell.SetRight(
        year_cell.Left() +
        (static_cast<float>(covered_days) * ctx.layout.DayWidth()));
    coverage_cells.push_back(coverage_cell);

    const float percent = static_cast<float>(covered_days) /
                          static_cast<float>(year.LengthDays());

    std::ostringstream coverage_stream;
    coverage_stream << std::fixed << std::setprecision(1)
                    << percent * detail::kPercentScale << " %";
    const auto coverage_text = coverage_stream.str();
    const float text_size = ctx.layout.BandSubHeight(0);

    RectF coverage_text_cell = coverage_cell;
    coverage_text_cell.SetLeft(coverage_cell.Right() + text_size);
    coverage_text_cell.SetRight(coverage_text_cell.Left() +
                                ctx.font->TextWidth(coverage_text, text_size));

    detail::SetCenteredText(
        ctx, coverage_labels,
        std::string("annual coverage label ") + std::to_string(index),
        coverage_text, coverage_text_cell.Center(), text_size);
  }

  detail::FillRectangles(node_cells, coverage_cells,
                         ctx.shape_config.GetShapeConfiguration(
                             ShapeConfigSet::kAnnualCoverageKey));
}

}  // namespace calendar_sections
