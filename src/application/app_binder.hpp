#ifndef APP_BINDER_HPP
#define APP_BINDER_HPP

#include <QtCore/QObject>

#include "../domain/calendar_config_store.hpp"
#include "../domain/date_entry_store.hpp"
#include "../domain/date_group_store.hpp"
#include "../domain/page_setup_store.hpp"
#include "../domain/shape_configuration_store.hpp"
#include "../domain/title_config_store.hpp"
#include "../domain/transform_date_entry.hpp"
#include "../presentation/calendar_panel.hpp"
#include "../presentation/date_panel.hpp"
#include "../presentation/document_panel.hpp"
#include "../presentation/font_panel.hpp"
#include "../presentation/gl_canvas.hpp"
#include "../presentation/groups_panel.hpp"
#include "../presentation/page_panel.hpp"
#include "../presentation/scene_tree_panel.hpp"
#include "../presentation/shape_panel.hpp"
#include "../presentation/title_panel.hpp"
#include "calendar/calendar_page.hpp"
#include "calendar/interaction_controller.hpp"
#include "calendar/title_text_editor.hpp"
#include "event_bus.hpp"
#include "state_burst.hpp"

// Everything the wiring reaches: stores, panels, rendering adapter and GL
// canvas. Which of them may send what to whom — a panel edit is a command to
// its store, the new state a fact on the bus — stands in AGENTS.md, "Event
// flow".
struct AppComponents {
  DateGroupStore& date_groups_store;
  DateEntryStore& date_entry_store;
  TransformDateEntry& transform_date_entry;
  PageSetupStore& page_setup_store;
  TitleConfigStore& title_config_store;
  ShapeConfigurationStore& shape_configuration_store;
  CalendarConfigStore& calendar_configuration_store;

  DateTablePanel& data_table_panel;
  DateGroupsTablePanel& date_groups_table_panel;
  DocumentSetupPanel& document_setup_panel;
  PageSetupPanel& page_setup_panel;
  TitleSetupPanel& title_setup_panel;
  CalendarSetupPanel& calendar_setup_panel;
  FontPanel& font_panel;
  SceneTreePanel& scene_tree_panel;
  ShapeSetupPanel& shape_setup_panel;

  CalendarPage& calendar_page;
  GLCanvas& gl_canvas;
  InteractionController& interaction_controller;
  TitleTextEditor& title_text_editor;
};

namespace app_binder {

// Connects every producer to its consumer: panel edits go to the owning store,
// the stores' facts go over the bus. Every connection carries `scope` as its
// context object and thereby lives exactly as long as that object — nothing
// here has to be disconnected by hand.
void Bind(QObject& scope, EventBus& bus, AppComponents& components);

// Releases what no context object can reach: the plain `std::function`
// callbacks of canvas, controller and editor. They are queries as much as
// notifications (a pick returns a hit), so they are no signals and Qt knows
// nothing of them — while they capture the rendering adapter, which gets
// dissolved right after the wiring.
void ReleaseCallbacks(AppComponents& components);

void SendInitialValues(EventBus& bus, AppComponents& components);

}  // namespace app_binder

// The wiring as a lifetime instead of two calls paired by hand: it connects on
// construction and releases on destruction. Declared as the last member of the
// composition root it dies before stores and bus — exactly the order the
// release needs.
class AppWiring {
 public:
  AppWiring(EventBus& bus, AppComponents components);
  ~AppWiring();
  AppWiring(const AppWiring&) = delete;
  AppWiring& operator=(const AppWiring&) = delete;
  AppWiring(AppWiring&&) = delete;
  AppWiring& operator=(AppWiring&&) = delete;

 private:
  AppComponents components_;
  // The context object of every connection Bind makes. Qt releases them all
  // when it dies, so no counterpart to Bind has to list them.
  QObject connection_scope_;
};

#endif  // APP_BINDER_HPP
