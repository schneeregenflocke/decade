#ifndef CALENDAR_PAGE_HPP
#define CALENDAR_PAGE_HPP

#include <memory>
#include <string>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/rect.hpp"
#include "../../gui/opengl_panel.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date_store.hpp"
#include "../../packages/group_store.hpp"
#include "../../packages/page_config.hpp"
#include "../../packages/shape_config.hpp"
#include "../../packages/title_config.hpp"
#include "calendar_scene_builder.hpp"

// Rendering adapter: owns the domain state relevant to the calendar drawing,
// receives store updates via the Receive* slots, and drives the
// CalendarSceneBuilder to (re)build the scene graph. The config state is held
// as plain value objects (copyable); the incoming signals still carry the
// owning stores, from which we extract the value.
class CalendarPage {
 public:
  CalendarPage(GLCanvas* gl_canvas_in, const std::string& font_filepath)
      : gl_canvas(gl_canvas_in),
        font(std::make_shared<Font>(font_filepath)),
        scene_builder(gl_canvas_in->GraphicsEnginePtr(), font, page_size,
                      page_margin, title_config, calendar_config, shape_config,
                      date_groups, data_store) {}

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups_in) {
    date_groups.Assign(date_groups_in);
    data_store.ReceiveDateGroups(date_groups_in);
    Update();
  }

  void ReceiveDateIntervalBundles(
      const std::vector<DateIntervalBundle>& date_interval_bundles) {
    data_store.ReceiveDateIntervalBundles(date_interval_bundles);
    Update();
  }

  void ReceivePageSetup(const PageSetupConfig& page_setup_config) {
    this->page_size = rectf::from_dimension(
        rectf::Dimension{.width = page_setup_config.size[0],
                         .height = page_setup_config.size[1]});
    this->page_margin =
        rectf(page_setup_config.margins[0], page_setup_config.margins[1],
              page_setup_config.margins[2], page_setup_config.margins[3]);
    Update();
  }

  void ReceiveFont(const std::string& font_filepath) {
    font = std::make_shared<Font>(font_filepath);
    Update();
  }

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    title_config = incoming_title_config;
    Update();
  }

  void ReceiveCalendarConfig(
      const CalendarConfigStorage& incoming_calendar_config) {
    calendar_config = incoming_calendar_config.Value();
    Update();
  }

  void ReceiveShapeConfigurationStorage(
      const ShapeConfigurationStorage& incoming_shape_configuration_storage) {
    shape_config = incoming_shape_configuration_storage.Value();
    Update();
  }

  void Update() {
    scene_builder.Build();
    gl_canvas->RefreshMVP();
  }

 private:
  GLCanvas* gl_canvas{nullptr};

  std::shared_ptr<Font> font;
  rectf page_size;
  rectf page_margin;

  DateIntervalBundleBarStore data_store;
  DateGroups date_groups;
  CalendarConfig calendar_config;
  ShapeConfigSet shape_config;
  TitleConfig title_config;

  // Declared last: binds references to the value members above, which must
  // already be constructed when the builder is initialised.
  CalendarSceneBuilder scene_builder;
};
#endif  // CALENDAR_PAGE_HPP
