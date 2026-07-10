#ifndef CALENDAR_SECTION_BUILDERS_HPP
#define CALENDAR_SECTION_BUILDERS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/pick_id.hpp"
#include "../../graphics/rect.hpp"
#include "../../graphics/scene_graph.hpp"
#include "../../graphics/scene_shape_filler.hpp"
#include "../../graphics/shaders.hpp"
#include "../../graphics/shapes.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_entry_bar_store.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../domain/title_config.hpp"
#include "calendar_layout.hpp"
#include "calendar_scene_nodes.hpp"

// Application/Infrastructure bridge: one free "section builder" per calendar
// element (title, grid cells, bars, legend, …). Each (re)fills its scene nodes
// from the domain state and the precomputed CalendarLayout. Splitting these out
// of CalendarSceneComposer keeps that class a thin coordinator and gives every
// element a single, self-contained place to live.
//
// The shared inputs travel in a SectionContext (all by reference / non-owning),
// so each function takes one context plus, where it produces something the
// coordinator needs (the bars' pick boxes and per-index nodes), an explicit
// result.
namespace calendar_sections {

// Bundles the references every section needs. Constructed per Build() by the
// coordinator; never stored.
struct SectionContext {
  const CalendarSceneNodes& nodes;
  const CalendarLayout& layout;
  const ShapeConfigSet& shape_config;
  const CalendarConfig& calendar_config;
  const TitleConfig& title_config;
  const DateGroups& date_groups;
  const DateEntryBarStore& bar_store;
  const std::shared_ptr<Font>& font;
  Shader* rectangles_shader;
  Shader* font_shader;
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

// Calendar-specific adapter over scene_shapes::FillRectangles: maps a domain
// ShapeConfiguration to the generic primitives and records the style id so the
// scene tree can route an edit back to that configuration.
template <typename Shapes>
inline void FillRectangles(const std::shared_ptr<SceneNode>& node,
                           const Shapes& shapes,
                           const ShapeConfiguration& config) {
  scene_shapes::FillRectangles(node, shapes, config.OutlineColor(),
                               config.FillColor(), config.LineWidth());
  node->SetStyleId(config.Name());
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

inline void BuildPrintArea(const SectionContext& ctx) {
  detail::FillRectangles(ctx.nodes.print_area, ctx.layout.PrintArea(),
                         ctx.shape_config.GetShapeConfiguration("Page Margin"));
}

inline void BuildTitle(const SectionContext& ctx) {
  detail::FillRectangles(ctx.nodes.title_frame, ctx.layout.TitleFrame(),
                         ctx.shape_config.GetShapeConfiguration("Title Frame"));

  auto title_shape =
      std::dynamic_pointer_cast<FontShape>(ctx.nodes.title_text->GetShape());
  if (!title_shape) {
    return;
  }
  title_shape->SetFont(ctx.font);
  title_shape->SetColor(ctx.title_config.TextColor());
  title_shape->SetShapeCentered(
      ctx.title_config.TitleText(), ctx.layout.TitleFrame().getCenter(),
      ctx.layout.TitleFrame().height() * ctx.title_config.FontSizeRatio());
}

inline void BuildCalendarLabels(const SectionContext& ctx) {
  constexpr size_t number_months = 12;
  std::array<char, detail::kMonthNameBufferSize> buf{};
  constexpr const char* format = "%b";
  std::array<std::string, number_months> months_names;

  for (size_t index = 0; index < months_names.size(); ++index) {
    std::tm month_tm = {};
    month_tm.tm_mon = static_cast<int>(index);

    if (std::strftime(buf.data(), std::size(buf), format, &month_tm) != 0) {
      months_names.at(index) = buf.data();
    }
  }

  std::vector<rectf> x_label_frames(number_months);
  const float labels_font_size = ctx.font->AdjustTextSize(
      rectf::from_dimension(rectf::Dimension{.width = ctx.layout.CellWidth(),
                                             .height = ctx.layout.RowHeight()}),
      "00000",
      Font::TextScale{.height_ratio = detail::kFontScaleMin,
                      .width_ratio = detail::kFontScaleMax});

  const auto& month_node = ctx.nodes.month_labels;

  month_node->RemoveChildren();
  for (size_t index = 0; index < number_months; ++index) {
    const auto float_index = static_cast<float>(index);
    const auto left =
        ctx.layout.XLabelsFrame().l() + (ctx.layout.CellWidth() * float_index);
    x_label_frames.at(index).setL(left);
    x_label_frames.at(index).setR(left + ctx.layout.CellWidth());
    x_label_frames.at(index).setB(ctx.layout.XLabelsFrame().b());
    x_label_frames.at(index).setT(ctx.layout.XLabelsFrame().t());

    detail::AddCenteredText(
        ctx, month_node, months_names.at(index), months_names.at(index),
        x_label_frames.at(index).getCenter(), labels_font_size);
  }

  const auto config = ctx.shape_config.GetShapeConfiguration("Calendar Labels");
  detail::FillRectangles(ctx.nodes.column_labels, x_label_frames, config);

  const auto& year_node = ctx.nodes.year_labels;

  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  if (span_years == 0) {
    return;
  }

  const TimelineProjection projection(ctx.calendar_config);
  std::vector<rectf> y_labels_frames(span_years);
  year_node->RemoveChildren();
  for (std::size_t index = 0; index < span_years; ++index) {
    const std::string current_year_text =
        std::to_string(projection.YearForRow(index));

    const auto float_index = static_cast<float>(index);
    const auto bottom =
        ctx.layout.YLabelsFrame().b() + (ctx.layout.RowHeight() * float_index);
    y_labels_frames.at(index).setL(ctx.layout.YLabelsFrame().l());
    y_labels_frames.at(index).setR(ctx.layout.YLabelsFrame().r());
    y_labels_frames.at(index).setB(bottom);
    y_labels_frames.at(index).setT(bottom + ctx.layout.RowHeight());

    detail::AddCenteredText(
        ctx, year_node, current_year_text, current_year_text,
        y_labels_frames.at(index).getCenter(), labels_font_size);
  }

  detail::FillRectangles(ctx.nodes.row_labels, y_labels_frames, config);
}

inline void BuildYears(const SectionContext& ctx) {
  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  if (span_years == 0) {
    return;
  }

  const TimelineProjection projection(ctx.calendar_config);
  std::vector<rectf> year_cells(span_years);

  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year = projection.YearForRow(index);
    const auto number_days = DaysInYear(current_year);
    const float year_length =
        static_cast<float>(number_days) * ctx.layout.DayWidth();
    rectf year_cell = ctx.layout.GetSubFrame(index, 1);
    year_cell.setR(year_cell.l() + year_length);
    year_cells.at(index) = year_cell;
  }

  detail::FillRectangles(
      ctx.nodes.year_cells, year_cells,
      ctx.shape_config.GetShapeConfiguration("Years Shapes"));
}

inline void BuildMonths(const SectionContext& ctx) {
  constexpr size_t number_months = 12;
  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  if (span_years == 0) {
    return;
  }

  const auto store_size = number_months * span_years;
  const TimelineProjection projection(ctx.calendar_config);
  std::vector<rectf> month_cells(store_size);

  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year = projection.YearForRow(index);
    const Date first_day_of_year = Date::FromYmd(current_year, 1, 1);

    for (size_t subindex = 0; subindex < number_months; ++subindex) {
      const auto current_cell = ctx.layout.GetSubFrame(index, 1);
      const int month_index = static_cast<int>(subindex);
      rectf month_cell;
      const auto start_offset =
          static_cast<float>(Date::DaysBetween(
              first_day_of_year, first_day_of_year.AddMonths(month_index))) *
          ctx.layout.DayWidth();
      const auto end_offset =
          static_cast<float>(
              Date::DaysBetween(first_day_of_year,
                                first_day_of_year.AddMonths(month_index + 1))) *
          ctx.layout.DayWidth();
      month_cell.setL(current_cell.l() + start_offset);
      month_cell.setR(current_cell.l() + end_offset);
      month_cell.setB(current_cell.b());
      month_cell.setT(current_cell.t());

      const auto store_index = (index * number_months) + subindex;
      month_cells.at(store_index) = month_cell;
    }
  }

  detail::FillRectangles(
      ctx.nodes.month_cells, month_cells,
      ctx.shape_config.GetShapeConfiguration("Months Shapes"));
}

inline void BuildDays(const SectionContext& ctx) {
  if (!ctx.calendar_config.IsValidSpan()) {
    return;
  }

  const auto span_days = ctx.calendar_config.GetSpanLengthDays();
  if (span_days <= 0) {
    return;
  }

  std::int64_t days_index = 0;
  const auto number_days_cells = static_cast<size_t>(span_days);

  std::vector<rectf> day_cells;
  std::vector<rectf> sunday_cells;
  day_cells.resize(number_days_cells);
  sunday_cells.resize(number_days_cells);

  const std::size_t span_years = ctx.calendar_config.GetSpanLengthYears();
  const TimelineProjection projection(ctx.calendar_config);
  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year = projection.YearForRow(index);
    const std::int64_t number_days = DaysInYear(current_year);

    for (std::int64_t subindex = 0; subindex < number_days; ++subindex) {
      const auto float_subindex = static_cast<float>(subindex);
      const auto current_cell = ctx.layout.GetSubFrame(index, 1);

      const Date current_date =
          ctx.calendar_config.GetSpanLimitsDate().at(0).AddDays(
              static_cast<int>(days_index));

      if (current_date.DayOfWeek() == Weekday::kSunday) {
        rectf day_cell;
        day_cell.setL(current_cell.l() +
                      (float_subindex * ctx.layout.DayWidth()));
        day_cell.setR(day_cell.l() + ctx.layout.DayWidth());
        day_cell.setB(current_cell.b());
        day_cell.setT(current_cell.t());
        sunday_cells[static_cast<size_t>(days_index)] = day_cell;
      } else {
        rectf day_cell;
        day_cell.setL(current_cell.l() +
                      (float_subindex * ctx.layout.DayWidth()));
        day_cell.setR(day_cell.l() + ctx.layout.DayWidth());
        day_cell.setB(current_cell.b());
        day_cell.setT(current_cell.t());
        day_cells[static_cast<size_t>(days_index)] = day_cell;
      }
      ++days_index;
    }
  }

  detail::FillRectangles(ctx.nodes.day_cells, day_cells,
                         ctx.shape_config.GetShapeConfiguration("Day Shapes"));
  detail::FillRectangles(
      ctx.nodes.sunday_cells, sunday_cells,
      ctx.shape_config.GetShapeConfiguration("Sunday Shapes"));
}

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
  const auto number_bars = ctx.bar_store.GetNumberBars();
  for (size_t index = 0; index < number_bars; ++index) {
    const auto& bar = ctx.bar_store.GetBar(index);
    if (!ctx.calendar_config.IsInSpan(bar.GetYear())) {
      continue;
    }
    const auto current_group = static_cast<size_t>(bar.GetGroup());
    auto current_shape_config =
        ctx.shape_config.GetDynamicConfiguration(current_group);

    const auto row = projection.RowForYear(bar.GetYear());
    const auto current_sub_cell = ctx.layout.GetSubFrame(row, 1);

    const auto bar_left =
        current_sub_cell.l() + (bar.GetFirstDay() * ctx.layout.DayWidth());
    const auto bar_width =
        (bar.GetLastDay() - bar.GetFirstDay()) * ctx.layout.DayWidth();
    const auto bar_height = current_sub_cell.height();

    // Each bar is its own node: the position lives in the node transform (ready
    // for dragging/animating), the size lives in the shape geometry. A pure
    // translation keeps the outline width constant, which a scale matrix would
    // distort. The bar's world rect is therefore unchanged.
    auto bar_node = std::make_shared<SceneNode>(std::string("bar ") +
                                                std::to_string(index));
    bar_node->SetModelMatrix(glm::translate(
        glm::mat4(1.0F),
        glm::vec3(bar_left, current_sub_cell.b(), detail::kZero)));
    bar_node->SetStyleId(current_shape_config.Name());

    // Page-space box for hit-testing. The node's world position is
    // layout.PrintAreaOrigin() + (bar_left, sub_cell.b()), so the page-space
    // rect is the local bar rect shifted by that origin.
    const PickId pick_id{.kind = PickId::Kind::kBar, .index = index};
    result.pick_boxes.push_back(PickBox{
        .id = pick_id,
        .rect = rectf(bar_left + ctx.layout.PrintAreaOrigin().x,
                      bar_left + bar_width + ctx.layout.PrintAreaOrigin().x,
                      current_sub_cell.b() + ctx.layout.PrintAreaOrigin().y,
                      current_sub_cell.b() + bar_height +
                          ctx.layout.PrintAreaOrigin().y)});

    auto bar_shape = std::make_shared<RectanglesShape>(ctx.rectangles_shader);
    bar_shape->SetShape(
        rectf(detail::kZero, bar_width, detail::kZero, bar_height),
        current_shape_config.LineWidth());
    bar_shape->SetColor({current_shape_config.OutlineColor(),
                         current_shape_config.FillColor()});
    bar_node->SetShape(bar_shape);
    bar_node->SetDrawLayer(calendar_layers::kBars);
    group_nodes.at(current_group)->AddChild(bar_node);
    result.bar_nodes.emplace(index, bar_node);

    auto current_text_cell = ctx.layout.GetSubFrame(row, 2);
    current_text_cell.setL(bar_left);
    current_text_cell.setR(bar_left + bar_width);

    detail::AddCenteredText(ctx, node_labels,
                            std::string("label node ") + std::to_string(index),
                            bar.GetText(), current_text_cell.getCenter(),
                            current_text_cell.height());
  }

  return result;
}

inline void BuildYearTotals(const SectionContext& ctx) {
  const auto& node_cells = ctx.nodes.year_totals;
  const auto& node_text = ctx.nodes.year_total_labels;
  node_text->RemoveChildren();

  const std::size_t span_years = ctx.bar_store.GetSpan();
  if (span_years == 0) {
    return;
  }

  const TimelineProjection projection(ctx.calendar_config);
  std::vector<rectf> year_totals_cells(span_years);

  for (std::size_t index = 0; index < span_years; ++index) {
    const int current_year =
        ctx.bar_store.GetFirstYear() + static_cast<int>(index);
    if (ctx.calendar_config.IsInSpan(current_year)) {
      const auto row = projection.RowForYear(current_year);
      const auto current_cell = ctx.layout.GetSubFrame(row, 0);

      rectf year_total_cell = current_cell;
      const auto year_total_width =
          static_cast<float>(ctx.bar_store.GetAnnualTotal(index)) *
          ctx.layout.DayWidth();
      year_total_cell.setR(current_cell.l() + year_total_width);
      year_totals_cells.at(index) = year_total_cell;

      const auto number_days = DaysInYear(current_year);

      const float percent =
          static_cast<float>(ctx.bar_store.GetAnnualTotal(index)) /
          static_cast<float>(number_days);

      std::ostringstream year_total_stream;
      year_total_stream << std::fixed << std::setprecision(1)
                        << percent * detail::kPercentScale << " %";
      const auto year_total_text = year_total_stream.str();
      const auto year_total_text_width =
          ctx.font->TextWidth(year_total_text, year_total_cell.height());

      rectf year_total_text_cell;
      year_total_text_cell.setL(year_total_cell.r() + current_cell.height());
      year_total_text_cell.setR(year_total_text_cell.l() +
                                year_total_text_width);
      year_total_text_cell.setB(year_total_cell.b());
      year_total_text_cell.setT(year_total_cell.t());

      detail::AddCenteredText(
          ctx, node_text,
          std::string("year total label ") + std::to_string(index),
          year_total_text, year_total_text_cell.getCenter(),
          year_total_text_cell.height());
    }
  }

  detail::FillRectangles(
      node_cells, year_totals_cells,
      ctx.shape_config.GetShapeConfiguration("Years Totals"));
}

inline void BuildLegend(const SectionContext& ctx) {
  const auto& node_entries = ctx.nodes.legend_entries;
  node_entries->RemoveChildren();

  const auto& node_text = ctx.nodes.legend_labels;
  node_text->RemoveChildren();

  const size_t number_entry_frames = (ctx.date_groups.Items().size() + 1) * 2;
  std::vector<rectf> legend_entries_frames(number_entry_frames);
  const auto entries_width = ctx.layout.LegendFrame().width() /
                             static_cast<float>(number_entry_frames);

  for (size_t index = 0; index < number_entry_frames; ++index) {
    const auto float_index = static_cast<float>(index);
    const auto left =
        ctx.layout.LegendFrame().l() + (entries_width * float_index);
    legend_entries_frames.at(index) = ctx.layout.LegendFrame();
    legend_entries_frames.at(index).setL(left);
    legend_entries_frames.at(index).setR(left + entries_width);
  }

  std::vector<rectf> bar_cells;

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
        legend_entries_frames.at(label_index).getCenter(), legend_font_size);

    if (span_years > 0U) {
      const auto current_height = ctx.layout.GetSubFrame(0, 1).height();
      auto current_cell = legend_entries_frames.at(label_index + 1);
      const auto current_vertical_center = current_cell.getCenter()[1];
      current_cell.setB(current_vertical_center -
                        (current_height * detail::kHalf));
      current_cell.setT(current_vertical_center +
                        (current_height * detail::kHalf));
      bar_cells.emplace_back(current_cell);

      auto current_shape_config =
          ctx.shape_config.GetDynamicConfiguration(index);

      auto node_entry = std::make_shared<SceneNode>(std::string("legend bar ") +
                                                    std::to_string(index));
      node_entry->SetDrawLayer(calendar_layers::kBars);
      node_entry->SetStyleId(current_shape_config.Name());
      node_entries->AddChild(node_entry);

      auto entry_shape =
          std::make_shared<RectanglesShape>(ctx.rectangles_shader);
      node_entry->SetShape(entry_shape);

      entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
      entry_shape->SetColor({current_shape_config.OutlineColor(),
                             current_shape_config.FillColor()});
    }
  }

  {
    detail::AddCenteredText(
        ctx, node_text, std::string("legend label year total"), "Annual sum",
        legend_entries_frames.at(legend_entries_frames.size() - 2).getCenter(),
        legend_font_size);

    if (span_years > 0U) {
      const auto current_height = ctx.layout.GetSubFrame(0, 0).height();
      auto current_cell =
          legend_entries_frames.at(legend_entries_frames.size() - 1);
      const auto current_vertical_center = current_cell.getCenter()[1];
      current_cell.setB(current_vertical_center -
                        (current_height * detail::kHalf));
      current_cell.setT(current_vertical_center +
                        (current_height * detail::kHalf));
      bar_cells.emplace_back(current_cell);

      auto current_shape_config = ctx.shape_config.GetShapeConfiguration(
          ShapeConfigSet::AnnualSumConfigurationName());

      auto node_entry =
          std::make_shared<SceneNode>(std::string("legend bar annual sum"));
      node_entry->SetDrawLayer(calendar_layers::kBars);
      node_entry->SetStyleId(current_shape_config.Name());
      node_entries->AddChild(node_entry);

      auto entry_shape =
          std::make_shared<RectanglesShape>(ctx.rectangles_shader);
      node_entry->SetShape(entry_shape);

      entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
      entry_shape->SetColor({current_shape_config.OutlineColor(),
                             current_shape_config.FillColor()});
    }
  }
}

}  // namespace calendar_sections

#endif  // CALENDAR_SECTION_BUILDERS_HPP
