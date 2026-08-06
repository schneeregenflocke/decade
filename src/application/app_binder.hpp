#ifndef APP_BINDER_HPP
#define APP_BINDER_HPP

#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <vector>

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
#include "../presentation/title_panel.hpp"
#include "calendar/calendar_page.hpp"
#include "calendar/interaction_controller.hpp"
#include "calendar/title_text_editor.hpp"
#include "event_bus.hpp"

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

  CalendarPage& calendar_page;
  GLCanvas& gl_canvas;
  InteractionController& interaction_controller;
  TitleTextEditor& title_text_editor;
};

namespace app_binder {

namespace detail {

// Hangs a panel signal onto a bus topic. Needed only where the producer sits in
// presentation and therefore gets no topic injected.
template <typename Signal, typename Topic>
void Forward(Signal& from, Topic& to) {
  from.connect([&to](const auto& value) { to(value); });
}

inline void BindDateEntries(EventBus& bus, AppComponents& components) {
  // Panel -> store (user input)
  components.data_table_panel.SignalTableDateEntries().connect(
      &DateEntryStore::ReceiveDateEntries, &components.date_entry_store);

  // Topic -> consumers. The store publishes itself.
  bus.date_entries().connect(&DateTablePanel::ReceiveDateEntries,
                             &components.data_table_panel);
  bus.date_entries().connect(&TransformDateEntry::ReceiveDateEntries,
                             &components.transform_date_entry);

  // The transform adapter publishes on its own topic. No shift: DatePeriod is
  // half-open [begin, end) everywhere, so the end is exclusive already. The
  // earlier {end_days = 1} was a correction out of the old inclusive model and
  // made every bar one day too long.
  bus.transformed_date_entries().connect(&CalendarPage::ReceiveDateEntries,
                                         &components.calendar_page);
}

inline void BindDateGroups(EventBus& bus, AppComponents& components) {
  components.date_groups_table_panel.SignalTableDateGroups().connect(
      &DateGroupStore::ReceiveDateGroups, &components.date_groups_store);

  bus.date_groups().connect(&DateGroupsTablePanel::ReceiveDateGroups,
                            &components.date_groups_table_panel);
  bus.date_groups().connect(&DateEntryStore::ReceiveDateGroups,
                            &components.date_entry_store);
  bus.date_groups().connect(&DateTablePanel::ReceiveDateGroups,
                            &components.data_table_panel);
  // The store synthesises the per-group shape configurations out of the palette
  // and publishes them anew — that must run before the scene rebuild, which
  // reads the updated configurations off the bus.
  bus.date_groups().connect(&ShapeConfigurationStore::ReceiveDateGroups,
                            &components.shape_configuration_store);
  bus.date_groups().connect(&CalendarPage::ReceiveDateGroups,
                            &components.calendar_page);
}

inline void BindPageSetup(EventBus& bus, AppComponents& components) {
  components.page_setup_panel.SignalPageSetupConfig().connect(
      &PageSetupStore::ReceivePageSetup, &components.page_setup_store);

  bus.page_setup().connect(&PageSetupPanel::ReceivePageSetup,
                           &components.page_setup_panel);
  bus.page_setup().connect(&CalendarPage::ReceivePageSetup,
                           &components.calendar_page);
  bus.page_setup().connect(&GLCanvas::ReceivePageSetup, &components.gl_canvas);
}

// The file path comes from the document, not from a store: loading and saving
// alone change it. The display is a pure consumer.
inline void BindProjectFilePath(EventBus& bus, AppComponents& components) {
  bus.project_file_path().connect(&DocumentSetupPanel::ReceiveProjectFilePath,
                                  &components.document_setup_panel);
}

// The font has no store — the panel value goes straight onto the topic and from
// there to the renderer. Should the font ever get saved with the project, a
// FontStore steps into the same place as with the other topics.
inline void BindFont(EventBus& bus, AppComponents& components) {
  Forward(components.font_panel.SignalFontConfig(), bus.font_config());

  bus.font_config().connect(&CalendarPage::ReceiveFont,
                            &components.calendar_page);
}

inline void BindTitleConfig(EventBus& bus, AppComponents& components) {
  components.title_setup_panel.SignalTitleConfig().connect(
      &TitleConfigStore::ReceiveTitleConfig, &components.title_config_store);

  bus.title_config().connect(&TitleSetupPanel::ReceiveTitleConfig,
                             &components.title_setup_panel);
  bus.title_config().connect(&CalendarPage::ReceiveTitleConfig,
                             &components.calendar_page);
}

inline void BindShapeConfiguration(EventBus& bus, AppComponents& components) {
  bus.shape_config_set().connect(&CalendarPage::ReceiveShapeConfigSet,
                                 &components.calendar_page);
  bus.shape_config_set().connect(&SceneTreePanel::ReceiveShapeConfigSet,
                                 &components.scene_tree_panel);
}

inline void BindCalendarConfig(EventBus& bus, AppComponents& components) {
  components.calendar_setup_panel.SignalCalendarConfig().connect(
      &CalendarConfigStore::ReceiveCalendarConfig,
      &components.calendar_configuration_store);

  bus.calendar_config().connect(&CalendarSetupPanel::ReceiveCalendarConfig,
                                &components.calendar_setup_panel);
  bus.calendar_config().connect(&CalendarPage::ReceiveCalendarConfig,
                                &components.calendar_page);
}

// The rendering adapter publishes the scene snapshots itself; the scene tree
// panel is the only consumer. Its selection goes back the other way over the
// bus to the highlight in the renderer.
inline void BindSceneSnapshot(EventBus& bus, AppComponents& components) {
  bus.scene_snapshot().connect(&SceneTreePanel::ReceiveSceneSnapshot,
                               &components.scene_tree_panel);

  Forward(components.scene_tree_panel.SignalSelectedNode(),
          bus.selected_node());
  bus.selected_node().connect(&CalendarPage::ReceiveSelectedNode,
                              &components.calendar_page);
  bus.selected_node().connect(&SceneTreePanel::ReceiveSelectedNode,
                              &components.scene_tree_panel);
}

// Picking: the canvas reports pointer movements in page space, the controller
// tests them through the rendering adapter and publishes hover and selection
// itself. Click and double click go to the text editor first: while an edit is
// running it consumes them (set the cursor, select a word), otherwise the
// binder passes them on to the controller.
inline void BindInteraction(EventBus& bus, AppComponents& components) {
  auto* page = &components.calendar_page;
  auto* controller = &components.interaction_controller;
  auto* editor = &components.title_text_editor;

  const auto pick = [page](glm::vec2 point) { return page->Pick(point); };
  controller->SetPickSource(pick);
  controller->SetPathSource(
      [page](const PickId& picked) { return page->NodePathFor(picked); });
  editor->SetPickSource(pick);
  editor->SetCaretIndexSource(
      [page](glm::vec2 point) { return page->TitleCaretIndexAt(point); });

  components.gl_canvas.SetPointerMoveCallback(
      [controller](glm::vec2 point) { controller->OnPointerMove(point); });

  components.gl_canvas.SetPrimaryDownCallback(
      [controller, editor](glm::vec2 point, bool extend) {
        const auto selection = extend ? TextEditBuffer::Selection::kExtend
                                      : TextEditBuffer::Selection::kReplace;
        if (editor->OnPrimaryDown(point, selection)) {
          return;
        }
        controller->OnPrimaryDown(point);
      });

  components.gl_canvas.SetDoubleClickCallback(
      [controller, editor](glm::vec2 point) {
        if (editor->OnDoubleClick(point)) {
          return;
        }
        controller->OnDoubleClick(point);
      });

  components.gl_canvas.SetTextInputCallback(
      [editor](const TextInputEvent& event) { editor->Handle(event); });
  components.gl_canvas.SetEditingQuery(
      [editor]() { return editor->IsEditing(); });
  components.gl_canvas.SetSelectedTextSource(
      [editor]() { return editor->SelectedText(); });

  bus.hovered().connect(&CalendarPage::ReceiveHovered,
                        &components.calendar_page);
  bus.edit_requested().connect(&TitleTextEditor::Begin, editor);
  bus.text_edit().connect(&CalendarPage::ReceiveTextEdit,
                          &components.calendar_page);
}

}  // namespace detail

inline void Bind(EventBus& bus, AppComponents& components) {
  detail::BindDateEntries(bus, components);
  detail::BindDateGroups(bus, components);
  detail::BindPageSetup(bus, components);
  detail::BindProjectFilePath(bus, components);
  detail::BindFont(bus, components);
  detail::BindTitleConfig(bus, components);
  detail::BindShapeConfiguration(bus, components);
  detail::BindCalendarConfig(bus, components);
  detail::BindSceneSnapshot(bus, components);
  detail::BindInteraction(bus, components);
}

// The counterpart to Bind: it disconnects everything. Needed on shutdown — the
// wx children (panels, GL canvas) die in the ~wxFrame base destructor alone, so
// after the stores and the event bus. Should a control still fire an event
// while being destroyed (an open editor commit, say), the slot would otherwise
// run into objects already destroyed.
inline void Unbind(EventBus& bus, AppComponents& components) {
  // The event sources of the presentation layer.
  components.data_table_panel.SignalTableDateEntries().disconnect_all();
  components.date_groups_table_panel.SignalTableDateGroups().disconnect_all();
  components.page_setup_panel.SignalPageSetupConfig().disconnect_all();
  components.title_setup_panel.SignalTitleConfig().disconnect_all();
  components.calendar_setup_panel.SignalCalendarConfig().disconnect_all();
  components.font_panel.SignalFontConfig().disconnect_all();
  components.scene_tree_panel.SignalSelectedNode().disconnect_all();
  components.gl_canvas.SetPointerMoveCallback(nullptr);
  components.gl_canvas.SetPrimaryDownCallback(nullptr);
  components.gl_canvas.SetDoubleClickCallback(nullptr);
  components.gl_canvas.SetTextInputCallback(nullptr);
  components.gl_canvas.SetEditingQuery(nullptr);
  components.gl_canvas.SetSelectedTextSource(nullptr);
  components.interaction_controller.SetPickSource(nullptr);
  components.interaction_controller.SetPathSource(nullptr);
  components.title_text_editor.SetPickSource(nullptr);
  components.title_text_editor.SetCaretIndexSource(nullptr);

  // Bus topics: their slots point at panels and the rendering adapter. The
  // producers may keep publishing afterwards — only nobody listens.
  bus.date_entries().disconnect_all();
  bus.transformed_date_entries().disconnect_all();
  bus.date_groups().disconnect_all();
  bus.page_setup().disconnect_all();
  bus.project_file_path().disconnect_all();
  bus.font_config().disconnect_all();
  bus.title_config().disconnect_all();
  bus.shape_config_set().disconnect_all();
  bus.calendar_config().disconnect_all();
  bus.scene_snapshot().disconnect_all();
  bus.hovered().disconnect_all();
  bus.selected_node().disconnect_all();
  bus.edit_requested().disconnect_all();
  bus.text_edit().disconnect_all();
}

inline void SendInitialValues(AppComponents& components) {
  components.shape_configuration_store.SendShapeConfigSet();
  components.date_groups_store.SendDefaultValues();
  components.page_setup_panel.SendDefaultValues();
  components.title_setup_panel.SendDefaultValues();
  components.calendar_configuration_store.SendCalendarConfig();
}

}  // namespace app_binder

// The wiring as a lifetime instead of two calls paired by hand: connect on
// construction, disconnect on destruction. Declared as the last member it dies
// before stores and bus — exactly the order the disconnect needs.
class AppWiring {
 public:
  AppWiring(EventBus& bus, AppComponents components)
      : bus_(bus), components_(components) {
    app_binder::Bind(bus_, components_);
    app_binder::SendInitialValues(components_);
  }
  ~AppWiring() { app_binder::Unbind(bus_, components_); }
  AppWiring(const AppWiring&) = delete;
  AppWiring& operator=(const AppWiring&) = delete;
  AppWiring(AppWiring&&) = delete;
  AppWiring& operator=(AppWiring&&) = delete;

 private:
  EventBus& bus_;
  AppComponents components_;
};

#endif  // APP_BINDER_HPP
