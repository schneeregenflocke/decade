#ifndef CALENDAR_SCENE_COMPOSER_HPP
#define CALENDAR_SCENE_COMPOSER_HPP

#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/graphics_engine.hpp"
#include "../../graphics/pick_id.hpp"
#include "../../graphics/scene.hpp"
#include "../../graphics/shapes.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date_entry_bar_store.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/title_config.hpp"
#include "calendar_layout.hpp"
#include "calendar_scene_nodes.hpp"
#include "calendar_section_builders.hpp"
#include "scene_highlighter.hpp"
#include "scene_snapshot.hpp"
#include "scene_snapshot_builder.hpp"

// Builds and fills the calendar scene graph from domain state. This is the
// rendering/layout half of the former CalendarPage: it borrows the Scene (whose
// skeleton it builds via BuildCalendarSceneNodes) and translates the
// (referenced) domain state into shapes. It is GL-canvas free — it only knows
// the GraphicsEngine and the Scene. CalendarPage owns the state and the Scene,
// and drives Build() in reaction to store updates.
class CalendarSceneComposer {
 public:
  CalendarSceneComposer(GraphicsEngine* graphics_engine_in, Scene& scene_in,
                        const std::shared_ptr<Font>& font_in,
                        const rectf& page_size_in, const rectf& page_margin_in,
                        const TitleConfig& title_config_in,
                        CalendarConfig& calendar_config_in,
                        const ShapeConfigSet& shape_config_in,
                        const DateGroups& date_groups_in,
                        const DateEntryBarStore& bar_store_in)
      : scene_(scene_in),
        graphics_engine_(graphics_engine_in),
        font_(font_in),
        page_size_(page_size_in),
        page_margin_(page_margin_in),
        title_config_(title_config_in),
        calendar_config_(calendar_config_in),
        shape_config_(shape_config_in),
        date_groups_(date_groups_in),
        bar_store_(bar_store_in) {
    graphics_engine_->SetScene(scene_);
    auto* simple_shader =
        graphics_engine_->SearchShader("Simple Shader").value_or(nullptr);
    rectangles_shader_ =
        graphics_engine_->SearchShader("Rectangles Shader").value_or(nullptr);
    font_shader_ =
        graphics_engine_->SearchShader("Font Shader").value_or(nullptr);

    // The fixed scene skeleton (named nodes, their painter layers and parent
    // attachments) is built once here; the handles drive the section builders.
    nodes_ = BuildCalendarSceneNodes(scene_, simple_shader, rectangles_shader_,
                                     font_shader_, font_);
  }

  void Build() {
    auto shape =
        std::dynamic_pointer_cast<QuadrilateralShape>(nodes_.page->GetShape());
    if (!shape) {
      return;
    }
    shape->SetShape(page_size_);
    shape->SetColor(glm::vec4(kOne, kOne, kOne, kOne));

    // The auto span derives the calendar's year range from the data; it must
    // run before the layout, which sizes the rows from the span length.
    if (calendar_config_.IsAutoCalendarSpan() && !bar_store_.is_empty()) {
      calendar_config_.SetSpan(
          CalendarSpan::YearSpan{.first_year = bar_store_.GetFirstYear(),
                                 .last_year = bar_store_.GetLastYear()});
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
    calendar_sections::BuildTitle(ctx);
    calendar_sections::BuildCalendarLabels(ctx);
    calendar_sections::BuildDays(ctx);
    calendar_sections::BuildMonths(ctx);
    calendar_sections::BuildYears(ctx);
    calendar_sections::BarSceneResult bars = calendar_sections::BuildBars(ctx);
    bar_pick_boxes_ = std::move(bars.pick_boxes);
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
        .bar_store = bar_store_,
        .font = font_,
        .rectangles_shader = rectangles_shader_,
        .font_shader = font_shader_};
  }

  // Plain, GL-free mirror of the current scene-graph hierarchy for the
  // presentation layer (the scene-tree widget). Rebuilt on demand from the
  // live graph after Build().
  [[nodiscard]] SceneNodeSnapshot SceneSnapshot() const {
    return BuildSceneSnapshot(scene_.Root());
  }

  // Page-space rectangles of the pickable bars, produced by the last Build().
  // Handed to the picking layer; Bullet-free.
  [[nodiscard]] const std::vector<PickBox>& BarPickBoxes() const {
    return bar_pick_boxes_;
  }

  // Hover and scene-tree selection highlighting are delegated to the
  // SceneHighlighter; the builder just forwards.
  void SetHoveredBar(const std::optional<PickId>& hovered) {
    highlighter_.SetHoveredBar(hovered);
  }

  void SetSelectedNode(const std::optional<std::string>& path) {
    highlighter_.SetSelectedNode(path);
  }

 private:
  static constexpr float kOne = 1.0F;

  // The scene graph's owner is the Scene (held by CalendarPage); the builder
  // borrows it to mutate the graph. It is not owned here.
  Scene& scene_;
  GraphicsEngine* graphics_engine_{nullptr};

  // Cached shader handles, looked up once in the constructor. The shaders live
  // in the GraphicsEngine for the builder's whole lifetime; they are forwarded
  // to the section builders via the SectionContext.
  Shader* rectangles_shader_{nullptr};
  Shader* font_shader_{nullptr};

  // Stable handles to the fixed scene-skeleton nodes, built once by
  // BuildCalendarSceneNodes and forwarded to the section builders.
  CalendarSceneNodes nodes_;

  // State owned by CalendarPage, referenced here. The referenced objects stay
  // alive and stable for the builder's lifetime; only their contents change.
  const std::shared_ptr<Font>& font_;
  const rectf& page_size_;
  const rectf& page_margin_;
  const TitleConfig& title_config_;
  CalendarConfig& calendar_config_;
  const ShapeConfigSet& shape_config_;
  const DateGroups& date_groups_;
  const DateEntryBarStore& bar_store_;

  // Transient render state, recomputed on every Build(). The page geometry now
  // lives in CalendarLayout; the builder only keeps what the sections produce.
  CalendarLayout layout_;
  std::vector<PickBox> bar_pick_boxes_;

  // Interactive hover/selection highlighting. Declared last so its borrowed
  // references (scene_, the overlay node, shape_config_, bar_store_) are all
  // initialised first; fed the fresh bar nodes via Refresh() after each
  // Build().
  SceneHighlighter highlighter_{scene_, nodes_.selection_overlay, shape_config_,
                                bar_store_};
};
#endif  // CALENDAR_SCENE_COMPOSER_HPP
