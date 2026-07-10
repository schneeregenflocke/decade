#ifndef CALENDAR_PAGE_HPP
#define CALENDAR_PAGE_HPP

#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/pick_id.hpp"
#include "../../graphics/rect.hpp"
#include "../../graphics/scene.hpp"
#include "../../gui/opengl_panel.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/date_entry.hpp"
#include "../../domain/date_entry_bar_store.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/page_setup_config.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/title_config.hpp"
#include "../../physics/physics_world.hpp"
#include "calendar_scene_composer.hpp"
#include "scene_snapshot.hpp"

// Rendering adapter: owns the domain state relevant to the calendar drawing,
// receives updates via the Receive* slots, and drives the CalendarSceneComposer
// to (re)build the scene graph. All state is held as plain value objects
// (copyable); every incoming signal carries a value, so the slots just assign.
class CalendarPage {
 public:
  CalendarPage(GLCanvas* gl_canvas_in, const std::string& font_filepath)
      : gl_canvas_(gl_canvas_in),
        font_(std::make_shared<Font>(font_filepath)),
        scene_composer_(gl_canvas_in->GraphicsEnginePtr(), scene_, font_,
                        page_size_, page_margin_, title_config_,
                        calendar_config_, shape_config_, date_groups_,
                        bar_store_) {}

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups_in) {
    date_groups_.Assign(date_groups_in);
    bar_store_.ReceiveDateGroups(date_groups_in);
    Update();
  }

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries) {
    bar_store_.ReceiveDateEntries(date_entries);
    Update();
  }

  void ReceivePageSetup(const PageSetupConfig& page_setup_config) {
    this->page_size_ = rectf::from_dimension(
        rectf::Dimension{.width = page_setup_config.Size()[0],
                         .height = page_setup_config.Size()[1]});
    this->page_margin_ =
        rectf(page_setup_config.Margins()[0], page_setup_config.Margins()[1],
              page_setup_config.Margins()[2], page_setup_config.Margins()[3]);
    Update();
  }

  void ReceiveFont(const std::string& font_filepath) {
    font_ = std::make_shared<Font>(font_filepath);
    Update();
  }

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    title_config_ = incoming_title_config;
    Update();
  }

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config) {
    calendar_config_ = incoming_calendar_config;
    Update();
  }

  void ReceiveShapeConfigSet(const ShapeConfigSet& incoming_shape_config_set) {
    shape_config_ = incoming_shape_config_set;
    Update();
  }

  void Update() {
    scene_composer_.Build();
    physics_world_.Rebuild(scene_composer_.BarPickBoxes());
    gl_canvas_->RefreshMVP();
    signal_scene_snapshot_(scene_composer_.SceneSnapshot());
  }

  [[nodiscard]] auto& SignalSceneSnapshot() { return signal_scene_snapshot_; }

  // Hit-tests a page-space point against the bars, returning the bar's PickId.
  [[nodiscard]] std::optional<PickId> Pick(glm::vec2 page_point) const {
    return physics_world_.Raycast(page_point);
  }

  // Highlights the hovered bar in place (no rebuild) and repaints. Only
  // colours change, so the cheap Repaint suffices — no projection refresh.
  void ReceiveHovered(const std::optional<PickId>& hovered) {
    scene_composer_.SetHoveredBar(hovered);
    gl_canvas_->Repaint();
  }

  // Highlights the scene-tree-selected node (and its subtree) in place and
  // repaints. The path identifies the node within the scene graph.
  void ReceiveSelectedNode(const std::optional<std::string>& path) {
    scene_composer_.SetSelectedNode(path);
    gl_canvas_->Repaint();
  }

 private:
  sigslot::signal<const SceneNodeSnapshot&> signal_scene_snapshot_;
  PhysicsWorld physics_world_;

  GLCanvas* gl_canvas_{nullptr};

  std::shared_ptr<Font> font_;
  rectf page_size_;
  rectf page_margin_;

  DateEntryBarStore bar_store_;
  DateGroups date_groups_;
  CalendarConfig calendar_config_;
  ShapeConfigSet shape_config_;
  TitleConfig title_config_;

  // The single owner (SSOT) of the render scene graph. Declared before the
  // builder, which borrows it; both outlive the GraphicsEngine's use of it.
  Scene scene_;

  // Declared last: binds references to the value members above, which must
  // already be constructed when the composer is initialised.
  CalendarSceneComposer scene_composer_;
};
#endif  // CALENDAR_PAGE_HPP
