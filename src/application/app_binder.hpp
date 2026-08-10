#ifndef APP_BINDER_HPP
#define APP_BINDER_HPP

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

// The one place where stores, panels, rendering adapter and GL canvas come
// together.
//
// Two directions, two rules:
//   * A panel edit is a *command* and goes straight to the owning store. Its
//     `Receive*` is thereby the only place canonical state comes into being.
//   * The new state is a *fact* and goes over the bus: the store publishes on
//     its topic, and every consumer attaches there.
// Were panels to put their edits onto the same topic they subscribe to, there
// would be feedback loops; the separation prevents that and keeps producer and
// consumer independent of each other.
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
// the stores' facts go over the bus.
void Bind(EventBus& bus, AppComponents& components);

// The counterpart to Bind: it disconnects everything. Needed on shutdown — the
// Qt children (panels, GL canvas) die in the ~QWidget base destructor alone, so
// after the stores and the event bus. Should a control still fire an event
// while being destroyed (an open editor commit, say), the slot would otherwise
// run into objects already destroyed.
void Unbind(EventBus& bus, AppComponents& components);

void SendInitialValues(EventBus& bus, AppComponents& components);

}  // namespace app_binder

// The wiring as a lifetime instead of two calls paired by hand: connect on
// construction, disconnect on destruction. Declared as the last member it dies
// before stores and bus — exactly the order the disconnect needs.
class AppWiring {
 public:
  AppWiring(EventBus& bus, AppComponents components);
  ~AppWiring();
  AppWiring(const AppWiring&) = delete;
  AppWiring& operator=(const AppWiring&) = delete;
  AppWiring(AppWiring&&) = delete;
  AppWiring& operator=(AppWiring&&) = delete;

 private:
  EventBus& bus_;
  AppComponents components_;
};

#endif  // APP_BINDER_HPP
