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
                        const DateEntryBars& date_entry_bars_in);

  void Build();

  // Bundles the references the section builders need into a context, built
  // fresh per Build() (never stored).
  [[nodiscard]] calendar_sections::SectionContext MakeContext() const;

  // Plain, GL-free mirror of the current scene-graph hierarchy for the
  // presentation layer (the scene-tree widget). Rebuilt on demand from the
  // live graph after Build().
  [[nodiscard]] SceneNodeSnapshot SceneSnapshot() const;

  // Page-space rectangles of the pickable elements (title, bars), produced by
  // the last Build(). Handed to the picking layer; Bullet-free.
  [[nodiscard]] const std::vector<PickBox>& PickBoxes() const;

  // Hover and scene-tree selection highlighting are delegated to the
  // SceneHighlighter; the builder just forwards.
  void SetHovered(const std::optional<PickId>& hovered);

  void SetSelectedNode(const std::optional<std::string>& path);

  // The state of the running text edit; the next Build() draws text, cursor and
  // selection out of it.
  void SetTextEdit(const std::optional<TextEditView>& text_edit);

  // The path "root/.../name" of the node a hit element means — the notion of
  // selection the scene tree uses too.
  [[nodiscard]] std::optional<std::string> NodePathFor(
      const PickId& picked) const;

  // The cursor index a click in page space means within the title line.
  [[nodiscard]] std::size_t TitleCaretIndexAt(glm::vec2 page_point) const;

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
                                             const std::string& name);

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
                                nodes_.title_area, shape_config_};
};
#endif  // CALENDAR_SCENE_COMPOSER_HPP
