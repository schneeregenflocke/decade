#ifndef CALENDAR_SCENE_BUILDER_HPP
#define CALENDAR_SCENE_BUILDER_HPP

#include <array>
#include <cstdint>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/graphics_engine.hpp"
#include "../../graphics/pick_id.hpp"
#include "../../graphics/scene.hpp"
#include "../../graphics/scene_graph.hpp"
#include "../../graphics/scene_shape_filler.hpp"
#include "../../graphics/shapes.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date.hpp"
#include "../../packages/date_entry_bar_store.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/timeline_projection.hpp"
#include "../../packages/title_config.hpp"
#include "calendar_layout.hpp"
#include "calendar_scene_nodes.hpp"
#include "calendar_section_builders.hpp"
#include "scene_snapshot.hpp"
#include "scene_snapshot_builder.hpp"

// Builds and fills the calendar scene graph from domain state. This is the
// rendering/layout half of the former CalendarPage: it borrows the Scene (whose
// skeleton it builds via BuildCalendarSceneNodes) and translates the
// (referenced) domain state into shapes. It is GL-canvas free — it only knows
// the GraphicsEngine and the Scene. CalendarPage owns the state and the Scene,
// and drives Build() in reaction to store updates.
class CalendarSceneBuilder {
 public:
  CalendarSceneBuilder(GraphicsEngine* graphics_engine_in, Scene& scene_in,
                       const std::shared_ptr<Font>& font_in,
                       const rectf& page_size_in, const rectf& page_margin_in,
                       const TitleConfig& title_config_in,
                       CalendarConfig& calendar_config_in,
                       const ShapeConfigSet& shape_config_in,
                       const DateGroups& date_groups_in,
                       const DateEntryBarStore& data_store_in)
      : scene_(scene_in),
        graphics_engine_(graphics_engine_in),
        font_(font_in),
        page_size_(page_size_in),
        page_margin_(page_margin_in),
        title_config_(title_config_in),
        calendar_config_(calendar_config_in),
        shape_config_(shape_config_in),
        date_groups_(date_groups_in),
        data_store_(data_store_in) {
    graphics_engine_->SetScene(scene_);
    auto* simple_shader =
        graphics_engine_->SearchShader("Simple Shader").value_or(nullptr);
    rectangles_shader_ =
        graphics_engine_->SearchShader("Rectangles Shader").value_or(nullptr);
    font_shader_ =
        graphics_engine_->SearchShader("Font Shader").value_or(nullptr);

    // The fixed scene skeleton (named nodes, their painter layers and parent
    // attachments) is built once here; the handles drive the Setup* methods.
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
    if (calendar_config_.IsAutoCalendarSpan() && !data_store_.is_empty()) {
      calendar_config_.SetSpan(
          CalendarSpan::YearSpan{.first_year = data_store_.GetFirstYear(),
                                 .last_year = data_store_.GetLastYear()});
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
    bar_nodes_ = std::move(bars.bar_nodes);
    calendar_sections::BuildYearTotals(ctx);
    calendar_sections::BuildLegend(ctx);

    // Re-apply the hover highlight to the freshly built bar nodes, if still
    // hovered, and the selection overlay whose target may have moved.
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/true);
    }
    ApplySelectionHighlight();
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
        .data_store = data_store_,
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

  // Highlights the hovered bar (and restores the previously hovered one) by
  // recolouring its shape in place — no scene rebuild. A null value clears the
  // highlight.
  void SetHoveredBar(const std::optional<PickId>& hovered) {
    if (hovered == hovered_bar_) {
      return;
    }
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/false);
    }
    hovered_bar_ = hovered;
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/true);
    }
  }

  // Highlights the scene node identified by `path` (and its subtree) with a
  // translucent overlay — no rebuild. A null/unknown path clears the overlay.
  void SetSelectedNode(const std::optional<std::string>& path) {
    selected_path_ = path;
    ApplySelectionHighlight();
  }

 private:
  // Positions the selection overlay over the currently selected node's world
  // bounds, or hides it (zero-area quad) when there is no resolvable selection.
  void ApplySelectionHighlight() {
    auto shape = std::dynamic_pointer_cast<QuadrilateralShape>(
        nodes_.selection_overlay->GetShape());
    if (!shape) {
      return;
    }
    std::optional<rectf> bounds;
    if (selected_path_.has_value()) {
      bounds = NodeWorldBounds(*selected_path_);
    }
    if (bounds.has_value()) {
      shape->SetShape(*bounds);
      shape->SetColor(glm::vec4(kSelectionRed, kSelectionGreen, kSelectionBlue,
                                kSelectionAlpha));
    } else {
      shape->SetShape(rectf(kZero, kZero, kZero, kZero));
    }
  }

  // Resolves a "root/.../name" path to the world-space bounds of that node's
  // subtree (page space, matching the bars' pick boxes). Returns nullopt when
  // any path segment does not resolve or the subtree carries no geometry.
  [[nodiscard]] std::optional<rectf> NodeWorldBounds(
      const std::string& path) const {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start <= path.size()) {
      const std::size_t slash = path.find('/', start);
      const std::size_t end =
          (slash == std::string::npos) ? path.size() : slash;
      segments.push_back(path.substr(start, end - start));
      if (slash == std::string::npos) {
        break;
      }
      start = slash + 1;
    }
    if (segments.empty() || scene_.Root().GetNodeName() != segments.front()) {
      return std::nullopt;
    }

    const SceneNode* node = &scene_.Root();
    glm::mat4 parent_world(1.0F);
    for (std::size_t index = 1; index < segments.size(); ++index) {
      parent_world = parent_world * node->GetModelMatrix();
      const SceneNode* next = nullptr;
      for (const auto& child : node->GetChildren()) {
        if (child->GetNodeName() == segments[index]) {
          next = child.get();
          break;
        }
      }
      if (next == nullptr) {
        return std::nullopt;
      }
      node = next;
    }
    return node->WorldBounds(parent_world);
  }

  // Recolours one bar's shape: highlighted bars get a distinct outline, normal
  // bars are restored to their group's configured colours. Fill is left as
  // configured so the hover reads as an outline accent.
  void ApplyBarColor(std::size_t bar_index, bool highlighted) {
    const auto iterator = bar_nodes_.find(bar_index);
    if (iterator == bar_nodes_.end() ||
        bar_index >= data_store_.GetNumberBars()) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(
        iterator->second->GetShape());
    if (!shape) {
      return;
    }
    const auto group =
        static_cast<std::size_t>(data_store_.GetBar(bar_index).GetGroup());
    const auto config = shape_config_.GetDynamicConfiguration(group);
    if (highlighted) {
      const glm::vec4 hover_outline(kOne, kHoverOutlineGreen, kZero, kOne);
      shape->SetColor({hover_outline, config.FillColor()});
    } else {
      shape->SetColor({config.OutlineColor(), config.FillColor()});
    }
  }

  static constexpr float kZero = 0.0F;
  static constexpr float kOne = 1.0F;
  static constexpr float kHoverOutlineGreen = 0.55F;

  // Translucent accent for the scene-tree selection overlay.
  static constexpr float kSelectionRed = 1.0F;
  static constexpr float kSelectionGreen = 0.6F;
  static constexpr float kSelectionBlue = 0.0F;
  static constexpr float kSelectionAlpha = 0.35F;

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
  const DateEntryBarStore& data_store_;

  // Bar nodes by bar index, rebuilt each Build() — used to recolour a bar on
  // hover. The hovered bar persists across rebuilds so the highlight is
  // re-applied to the fresh node.
  std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes_;
  std::optional<PickId> hovered_bar_;

  // Path of the scene-tree-selected node ("root/.../name"); drives the overlay
  // and persists across rebuilds so the highlight is re-applied to fresh nodes.
  std::optional<std::string> selected_path_;

  // Transient render state, recomputed on every Build(). The page geometry now
  // lives in CalendarLayout; the builder only keeps what the sections produce.
  CalendarLayout layout_;
  std::vector<PickBox> bar_pick_boxes_;
};
#endif  // CALENDAR_SCENE_BUILDER_HPP
