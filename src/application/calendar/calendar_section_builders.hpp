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
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../domain/calendar_config.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_entry_bars.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/font_config.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/timeline_projection.hpp"
#include "../../domain/title_config.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/scene_shape_filler.hpp"
#include "../../infrastructure/graphics/shaders.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
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
  const DateEntryBars& date_entry_bars;
  // Empty while nobody edits text in the canvas.
  const std::optional<TextEditView>& text_edit;
  const std::shared_ptr<Font>& font;
  // The application-wide chosen font including its point size.
  const FontConfig& font_config;
  Shader& rectangles_shader;
  Shader& font_shader;
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

// A calendar-side adapter over scene_shapes::FillRectangles: it maps a domain
// ShapeConfiguration onto the general primitives and notes the style ID at
// which the scene tree shows the values of the configuration.
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

// The geometry of the title line and, while editing, of cursor and selection.
// Both hang on the same computation — text width in code points, measured from
// the left of the centred text — which is why they sit together here.
namespace title_edit {

inline constexpr float kCaretWidthRatio = 0.06F;
inline constexpr float kSelectionRed = 0.25F;
inline constexpr float kSelectionGreen = 0.5F;
inline constexpr float kSelectionBlue = 1.0F;
inline constexpr float kSelectionAlpha = 0.35F;

// The text to draw right now, with its font size and left edge.
struct TextLine {
  std::string text;
  std::u32string code_points;
  float font_size{0.0F};
  float left{0.0F};
};

[[nodiscard]] inline TextLine Layout(const SectionContext& ctx) {
  TextLine line;
  line.text = ctx.text_edit.has_value() ? ctx.text_edit->text
                                        : ctx.title_config.TitleText();
  const std::vector<char32_t> decoded = DecodeUtf8(line.text);
  line.code_points.assign(decoded.begin(), decoded.end());
  line.font_size = ctx.title_config.FontSizeMillimetres();
  line.left = ctx.layout.TitleFrame().Center().x -
              (ctx.font->TextWidth(line.text, line.font_size) * detail::kHalf);
  return line;
}

// Sets the cursor and selection areas, or hides both (a null area) when nobody
// is editing.
inline void FillCaretAndSelection(const SectionContext& ctx,
                                  const TextLine& line) {
  auto* caret_shape =
      dynamic_cast<QuadrilateralShape*>(ctx.nodes.title_caret->GetShape());
  auto* selection_shape =
      dynamic_cast<QuadrilateralShape*>(ctx.nodes.title_selection->GetShape());
  if (caret_shape == nullptr || selection_shape == nullptr) {
    return;
  }
  const rectf hidden(detail::kZero, detail::kZero, detail::kZero,
                     detail::kZero);
  if (!ctx.text_edit.has_value()) {
    caret_shape->SetShape(hidden);
    selection_shape->SetShape(hidden);
    return;
  }

  const float text_height = ctx.font->TextHeight(line.font_size);
  const float center_y = ctx.layout.TitleFrame().Center().y;
  const float bottom = center_y - (text_height * detail::kHalf);
  const float top = center_y + (text_height * detail::kHalf);
  const auto offset = [&](std::size_t index) {
    return line.left +
           ctx.font->TextWidth(line.code_points, line.font_size, index);
  };

  const float caret_x = offset(ctx.text_edit->caret);
  const float caret_width = line.font_size * kCaretWidthRatio;
  caret_shape->SetShape(rectf(caret_x, caret_x + caret_width, bottom, top));
  caret_shape->SetColor(ctx.title_config.TextColor());

  if (HasSelection(*ctx.text_edit)) {
    selection_shape->SetShape(rectf(offset(ctx.text_edit->selection_begin),
                                    offset(ctx.text_edit->selection_end),
                                    bottom, top));
    selection_shape->SetColor(glm::vec4(kSelectionRed, kSelectionGreen,
                                        kSelectionBlue, kSelectionAlpha));
  } else {
    selection_shape->SetShape(hidden);
  }
}

// The inverse for the pointer: the cursor index a click in page space means.
// The point arrives in page space while the title geometry sits local to the
// print area — hence subtracting its origin.
[[nodiscard]] inline std::size_t CaretIndexAt(const SectionContext& ctx,
                                              const TextLine& line,
                                              glm::vec2 page_point) {
  const float local_x = page_point.x - ctx.layout.PrintAreaOrigin().x;
  return ctx.font->IndexAtOffset(line.code_points, line.font_size,
                                 local_x - line.left);
}

}  // namespace title_edit

inline void BuildPrintArea(const SectionContext& ctx) {
  detail::FillRectangles(
      ctx.nodes.print_area, ctx.layout.PrintArea(),
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kPageMargin));
}

// The title is a pickable element: its hit area is the frame the text fills. As
// with the bars, the returned box lies in page space, so shifted by the origin
// of the print area.
//
// During an edit the frame shows the buffer instead of the stored title — that
// becomes canonical with Enter alone — and cursor and selection alongside.
[[nodiscard]] inline PickBox BuildTitle(const SectionContext& ctx) {
  detail::FillRectangles(
      ctx.nodes.title_frame, ctx.layout.TitleFrame(),
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kTitleFrame));

  const title_edit::TextLine line = title_edit::Layout(ctx);
  if (auto* title_shape =
          dynamic_cast<FontShape*>(ctx.nodes.title_text->GetShape())) {
    title_shape->SetFont(ctx.font);
    title_shape->SetColor(ctx.title_config.TextColor());
    title_shape->SetShapeCentered(line.text, ctx.layout.TitleFrame().Center(),
                                  line.font_size);
  }
  title_edit::FillCaretAndSelection(ctx, line);

  return PickBox{
      .id = PickId{.kind = PickId::Kind::kTitle, .index = 0},
      .rect = ctx.layout.TitleFrame().Shift(ctx.layout.PrintAreaOrigin().x,
                                            ctx.layout.PrintAreaOrigin().y)};
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
  // Month names and year numbers carry the application-wide chosen size in
  // points — they label the page, not the individual bar, and should therefore
  // not travel with the cell size.
  const float labels_font_size = ctx.font_config.SizeMillimetres();

  const auto& month_node = ctx.nodes.month_labels;

  month_node->RemoveChildren();
  for (size_t index = 0; index < number_months; ++index) {
    const auto float_index = static_cast<float>(index);
    const auto left = ctx.layout.XLabelsFrame().Left() +
                      (ctx.layout.CellWidth() * float_index);
    x_label_frames.at(index).SetLeft(left);
    x_label_frames.at(index).SetRight(left + ctx.layout.CellWidth());
    x_label_frames.at(index).SetBottom(ctx.layout.XLabelsFrame().Bottom());
    x_label_frames.at(index).SetTop(ctx.layout.XLabelsFrame().Top());

    detail::AddCenteredText(
        ctx, month_node, months_names.at(index), months_names.at(index),
        x_label_frames.at(index).Center(), labels_font_size);
  }

  const auto config =
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kCalendarLabels);
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
    const auto bottom = ctx.layout.YLabelsFrame().Bottom() +
                        (ctx.layout.RowHeight() * float_index);
    y_labels_frames.at(index).SetLeft(ctx.layout.YLabelsFrame().Left());
    y_labels_frames.at(index).SetRight(ctx.layout.YLabelsFrame().Right());
    y_labels_frames.at(index).SetBottom(bottom);
    y_labels_frames.at(index).SetTop(bottom + ctx.layout.RowHeight());

    detail::AddCenteredText(
        ctx, year_node, current_year_text, current_year_text,
        y_labels_frames.at(index).Center(), labels_font_size);
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
    year_cell.SetRight(year_cell.Left() + year_length);
    year_cells.at(index) = year_cell;
  }

  detail::FillRectangles(
      ctx.nodes.year_cells, year_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kYearsShapes));
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
      month_cell.SetLeft(current_cell.Left() + start_offset);
      month_cell.SetRight(current_cell.Left() + end_offset);
      month_cell.SetBottom(current_cell.Bottom());
      month_cell.SetTop(current_cell.Top());

      const auto store_index = (index * number_months) + subindex;
      month_cells.at(store_index) = month_cell;
    }
  }

  detail::FillRectangles(
      ctx.nodes.month_cells, month_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kMonthsShapes));
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
        day_cell.SetLeft(current_cell.Left() +
                         (float_subindex * ctx.layout.DayWidth()));
        day_cell.SetRight(day_cell.Left() + ctx.layout.DayWidth());
        day_cell.SetBottom(current_cell.Bottom());
        day_cell.SetTop(current_cell.Top());
        sunday_cells[static_cast<size_t>(days_index)] = day_cell;
      } else {
        rectf day_cell;
        day_cell.SetLeft(current_cell.Left() +
                         (float_subindex * ctx.layout.DayWidth()));
        day_cell.SetRight(day_cell.Left() + ctx.layout.DayWidth());
        day_cell.SetBottom(current_cell.Bottom());
        day_cell.SetTop(current_cell.Top());
        day_cells[static_cast<size_t>(days_index)] = day_cell;
      }
      ++days_index;
    }
  }

  detail::FillRectangles(
      ctx.nodes.day_cells, day_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kDayShapes));
  detail::FillRectangles(
      ctx.nodes.sunday_cells, sunday_cells,
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kSundayShapes));
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

inline void BuildLegend(const SectionContext& ctx) {
  const auto& node_entries = ctx.nodes.legend_entries;
  node_entries->RemoveChildren();

  const auto& node_text = ctx.nodes.legend_labels;
  node_text->RemoveChildren();

  const size_t number_entry_frames = (ctx.date_groups.Items().size() + 1) * 2;
  std::vector<rectf> legend_entries_frames(number_entry_frames);
  const auto entries_width = ctx.layout.LegendFrame().Width() /
                             static_cast<float>(number_entry_frames);

  for (size_t index = 0; index < number_entry_frames; ++index) {
    const auto float_index = static_cast<float>(index);
    const auto left =
        ctx.layout.LegendFrame().Left() + (entries_width * float_index);
    legend_entries_frames.at(index) = ctx.layout.LegendFrame();
    legend_entries_frames.at(index).SetLeft(left);
    legend_entries_frames.at(index).SetRight(left + entries_width);
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
        legend_entries_frames.at(label_index).Center(), legend_font_size);

    if (span_years > 0U) {
      const auto current_height = ctx.layout.GetSubFrame(0, 1).Height();
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

      auto entry_shape =
          std::make_unique<RectanglesShape>(ctx.rectangles_shader);
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
      const auto current_height = ctx.layout.GetSubFrame(0, 0).Height();
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

      auto entry_shape =
          std::make_unique<RectanglesShape>(ctx.rectangles_shader);
      entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
      entry_shape->SetColors(current_shape_config.OutlineColor(),
                             current_shape_config.FillColor());
      node_entry->SetShape(std::move(entry_shape));
    }
  }
}

}  // namespace calendar_sections

#endif  // CALENDAR_SECTION_BUILDERS_HPP
