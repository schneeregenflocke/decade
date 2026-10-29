#include "bar_sections.hpp"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include "../../domain/band_part.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_period.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/timeline_projection.hpp"
#include "calendar_scene_nodes.hpp"
#include "graphics/child_pool.hpp"
#include "graphics/font.hpp"
#include "graphics/pick_id.hpp"
#include "graphics/rect.hpp"
#include "graphics/scene_shape_filler.hpp"
#include "graphics/shapes.hpp"
#include "section_context.hpp"

namespace calendar_sections {

BarSceneResult BuildBars(const SectionContext& ctx) {
  BarSceneResult result;

  // One group per entry: it carries the category style and holds the bar
  // pieces — one per row the entry reaches — beside their labels.
  ChildPool entry_pool(ctx.nodes.date_bars);
  for (size_t entry = 0; entry < ctx.date_entry_bars.GetNumberBars(); ++entry) {
    const auto& bar_data = ctx.date_entry_bars.GetBar(entry);
    const auto shape_config = ctx.shape_config.GetDynamicConfiguration(
        static_cast<size_t>(bar_data.GetCategory()));

    const auto entry_node = entry_pool.Next(EntryNodeName(entry));
    entry_node->SetStyleId(shape_config.Key());
    result.entry_nodes.push_back(entry_node);

    const std::vector<DatePeriod> pieces =
        ctx.projection.SplitAtRowBoundaries(bar_data.Period());
    ChildPool members(entry_node);
    for (size_t index = 0; index < pieces.size(); ++index) {
      const RectF bar_area =
          detail::PeriodArea(ctx, pieces[index], BandPart::kDays);

      // The position lives in the node transform (ready for dragging), the
      // size in the shape geometry. A pure translation keeps the outline width
      // constant, which a scale matrix would distort.
      const auto piece = members.NextShape<BoxesShape>(
          "piece " + std::to_string(index), calendar_layers::kBars,
          ctx.rectangles_shader);
      piece.node->SetModelMatrix(glm::translate(
          glm::mat4(1.0F),
          glm::vec3(bar_area.Left(), bar_area.Bottom(), detail::kZero)));
      piece.shape.SetShape(RectF(detail::kZero, bar_area.Width(), detail::kZero,
                                 bar_area.Height()),
                           shape_config.LineWidth());
      piece.shape.SetColors(shape_config.OutlineColor(),
                            shape_config.FillColor());

      // Page-space box for hit-testing: the piece's world position is
      // layout.PrintAreaOrigin() + the bar's corner.
      result.pick_boxes.push_back(
          PickBox{.id = {.kind = PickId::Kind::kBar, .index = entry},
                  .rect = bar_area.Shift(ctx.layout.PrintAreaOrigin().x,
                                         ctx.layout.PrintAreaOrigin().y)});
    }
    for (size_t index = 0; index < pieces.size(); ++index) {
      const auto label =
          members.NextShape<FontShape>("label " + std::to_string(index),
                                       calendar_layers::kText, ctx.font_shader);
      scene_shapes::SetCenteredText(
          label.shape, bar_data.GetText(),
          detail::PeriodArea(ctx, pieces[index], BandPart::kEntryLabels)
              .Center(),
          ctx.layout.BandPartHeight(BandPart::kEntryLabels), ctx.font);
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
    const RectF year_cell = detail::PeriodArea(ctx, year, BandPart::kCoverage);
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
    const float text_size = ctx.layout.BandPartHeight(BandPart::kCoverage);

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
