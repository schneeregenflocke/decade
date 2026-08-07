#ifndef CALENDAR_SCENE_COMPOSER_HPP
#define CALENDAR_SCENE_COMPOSER_HPP

#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../domain/calendar_config.hpp"
#include "../../domain/date_entry_bars.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/font_config.hpp"
#include "../../domain/scene_snapshot.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/title_config.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/graphics_engine.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "bar_sections.hpp"
#include "calendar_layout.hpp"
#include "calendar_scene_nodes.hpp"
#include "grid_sections.hpp"
#include "legend_section.hpp"
#include "scene_highlighter.hpp"
#include "scene_snapshot_builder.hpp"
#include "section_context.hpp"
#include "title_section.hpp"

// Builds and fills the calendar scene graph from domain state. This is the
// rendering/layout half of the former CalendarPage: it borrows the Scene (whose
// skeleton it builds via BuildCalendarSceneNodes) and translates the
// (referenced) domain state into shapes. It is GL-canvas free — it only knows
// the GraphicsEngine and the Scene. CalendarPage owns the state and the Scene,
// and drives Build() in reaction to store updates.
class CalendarSceneComposer {
 public:
  CalendarSceneComposer(GraphicsEngine& graphics_engine_in, Scene& scene_in,
                        const std::shared_ptr<Font>& font_in,
                        const FontConfig& font_config_in,
                        const RectF& page_size_in, const RectF& page_margin_in,
                        const TitleConfig& title_config_in,
                        CalendarConfig& calendar_config_in,
                        const ShapeConfigSet& shape_config_in,
                        const DateGroups& date_groups_in,
                        const DateEntryBars& date_entry_bars_in)
      : scene_(scene_in),
        graphics_engine_(graphics_engine_in),
        rectangles_shader_(
            RequireShader(graphics_engine_in, "Rectangles Shader")),
        font_shader_(RequireShader(graphics_engine_in, "Font Shader")),
        font_(font_in),
        font_config_(font_config_in),
        page_size_(page_size_in),
        page_margin_(page_margin_in),
        title_config_(title_config_in),
        calendar_config_(calendar_config_in),
        shape_config_(shape_config_in),
        date_groups_(date_groups_in),
        date_entry_bars_(date_entry_bars_in) {
    graphics_engine_.SetScene(scene_);
    Shader& simple_shader = RequireShader(graphics_engine_, "Simple Shader");

    // The fixed scene skeleton (named nodes, their painter layers and parent
    // attachments) is built once here; the handles drive the section builders.
    nodes_ = BuildCalendarSceneNodes(scene_, simple_shader, rectangles_shader_,
                                     font_shader_, font_);
  }

  void Build() {
    auto* shape = dynamic_cast<QuadrilateralShape*>(nodes_.page->GetShape());
    if (shape == nullptr) {
      return;
    }
    shape->SetShape(page_size_);
    shape->SetColor(glm::vec4(kOne, kOne, kOne, kOne));

    // The auto span derives the calendar's year range from the data; it must
    // run before the layout, which sizes the rows from the span length.
    if (calendar_config_.IsAutoCalendarSpan() && !date_entry_bars_.is_empty()) {
      calendar_config_.SetSpan(
          CalendarSpan::YearSpan{.first_year = date_entry_bars_.GetFirstYear(),
                                 .last_year = date_entry_bars_.GetLastYear()});
    }

    layout_ =
        CalendarLayout(page_size_, page_margin_, title_config_.FrameHeight(),
                       calendar_config_.GetSpanLengthYears(),
                       calendar_config_.GetSpacingProportions());

    // The print-area node carries the print area's offset within the page;
    // every descendant is computed in print-area-local coordinates (origin at
    // the print area's bottom-left). The page rectangle itself stays in
    // absolute page space on the untransformed page node above.
    nodes_.print_area->SetModelMatrix(
        glm::translate(glm::mat4(1.0F), layout_.PrintAreaOrigin()));

    const calendar_sections::SectionContext ctx = MakeContext();
    calendar_sections::BuildPrintArea(ctx);
    pick_boxes_.clear();
    pick_boxes_.push_back(calendar_sections::BuildTitle(ctx));
    calendar_sections::BuildCalendarLabels(ctx);
    calendar_sections::BuildDays(ctx);
    calendar_sections::BuildMonths(ctx);
    calendar_sections::BuildYears(ctx);
    calendar_sections::BarSceneResult bars = calendar_sections::BuildBars(ctx);
    pick_boxes_.insert(pick_boxes_.end(), bars.pick_boxes.begin(),
                       bars.pick_boxes.end());
    calendar_sections::BuildYearTotals(ctx);
    calendar_sections::BuildLegend(ctx);

    // Hand the fresh bar nodes to the highlighter, which re-applies the
    // persisted hover and selection highlights to the new geometry.
    highlighter_.Refresh(std::move(bars.bar_nodes));
  }

  // Bundles the references the section builders need into a context, built
  // fresh per Build() (never stored).
  [[nodiscard]] calendar_sections::SectionContext MakeContext() const {
    return calendar_sections::SectionContext{
        .nodes = nodes_,
        .layout = layout_,
        .shape_config = shape_config_,
        .calendar_config = calendar_config_,
        .title_config = title_config_,
        .date_groups = date_groups_,
        .date_entry_bars = date_entry_bars_,
        .text_edit = text_edit_,
        .font = font_,
        .font_config = font_config_,
        .rectangles_shader = rectangles_shader_,
        .font_shader = font_shader_};
  }

  // Plain, GL-free mirror of the current scene-graph hierarchy for the
  // presentation layer (the scene-tree widget). Rebuilt on demand from the
  // live graph after Build().
  [[nodiscard]] SceneNodeSnapshot SceneSnapshot() const {
    return BuildSceneSnapshot(scene_.Root());
  }

  // Page-space rectangles of the pickable elements (title, bars), produced by
  // the last Build(). Handed to the picking layer; Bullet-free.
  [[nodiscard]] const std::vector<PickBox>& PickBoxes() const {
    return pick_boxes_;
  }

  // Hover and scene-tree selection highlighting are delegated to the
  // SceneHighlighter; the builder just forwards.
  void SetHovered(const std::optional<PickId>& hovered) {
    highlighter_.SetHovered(hovered);
  }

  void SetSelectedNode(const std::optional<std::string>& path) {
    highlighter_.SetSelectedNode(path);
  }

  // The state of the running text edit; the next Build() draws text, cursor and
  // selection out of it.
  void SetTextEdit(const std::optional<TextEditView>& text_edit) {
    text_edit_ = text_edit;
  }

  // The path "root/.../name" of the node a hit element means — the notion of
  // selection the scene tree uses too.
  [[nodiscard]] std::optional<std::string> NodePathFor(
      const PickId& picked) const {
    const auto node = highlighter_.NodeFor(picked);
    if (!node) {
      return std::nullopt;
    }
    return FindNodePath(scene_.Root(), *node);
  }

  // The cursor index a click in page space means within the title line.
  [[nodiscard]] std::size_t TitleCaretIndexAt(glm::vec2 page_point) const {
    const calendar_sections::SectionContext ctx = MakeContext();
    return calendar_sections::title_edit::CaretIndexAt(
        ctx, calendar_sections::title_edit::Layout(ctx), page_point);
  }

 private:
  static constexpr float kOne = 1.0F;

  // The scene graph's owner is the Scene (held by CalendarPage); the builder
  // borrows it to mutate the graph. It is not owned here.
  Scene& scene_;
  GraphicsEngine& graphics_engine_;

  // Cached shader handles, looked up once in the constructor. The shaders live
  // in the GraphicsEngine for the builder's whole lifetime; they are forwarded
  // to the section builders via the SectionContext.
  // The three shaders the calendar draws with are built unconditionally by
  // `Shaders` out of embedded resources, so a miss here means a typo in the
  // name or a resource that went away — a fault, not a state to carry. It used
  // to fall to nullptr through `value_or`, and the null then died inside
  // Shape::SetShader without a word about which shader was missing. Naming it
  // and throwing lets AppComposition report and close down the same path it
  // already uses when the GL context fails to come up ([#53]).
  [[nodiscard]] static Shader& RequireShader(GraphicsEngine& graphics_engine,
                                             const std::string& name) {
    const auto found = graphics_engine.SearchShader(name);
    if (!found.has_value()) {
      throw std::runtime_error("shader not found: " + name);
    }
    return found->get();
  }

  Shader& rectangles_shader_;
  Shader& font_shader_;

  // Stable handles to the fixed scene-skeleton nodes, built once by
  // BuildCalendarSceneNodes and forwarded to the section builders.
  CalendarSceneNodes nodes_;

  // State owned by CalendarPage, referenced here. The referenced objects stay
  // alive and stable for the builder's lifetime; only their contents change.
  const std::shared_ptr<Font>& font_;
  const FontConfig& font_config_;
  const RectF& page_size_;
  const RectF& page_margin_;
  const TitleConfig& title_config_;
  CalendarConfig& calendar_config_;
  const ShapeConfigSet& shape_config_;
  const DateGroups& date_groups_;
  const DateEntryBars& date_entry_bars_;

  // Transient render state, recomputed on every Build(). The page geometry now
  // lives in CalendarLayout; the builder only keeps what the sections produce.
  CalendarLayout layout_;
  std::vector<PickBox> pick_boxes_;
  std::optional<TextEditView> text_edit_;

  // Interactive hover/selection highlighting. Declared last so its borrowed
  // references (scene_, the overlay and title nodes, shape_config_) are all
  // initialised first; fed the fresh bar nodes via Refresh() after each
  // Build().
  SceneHighlighter highlighter_{scene_, nodes_.selection_overlay,
                                nodes_.title_frame, shape_config_};
};
#endif  // CALENDAR_SCENE_COMPOSER_HPP
