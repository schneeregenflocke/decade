#include "app_binder.hpp"

#include <QtCore/QObject>
#include <glm/ext/vector_float2.hpp>

#include "../domain/calendar_config_store.hpp"
#include "../domain/date_entry_store.hpp"
#include "../domain/date_group_store.hpp"
#include "../domain/page_setup_store.hpp"
#include "../domain/shape_configuration_store.hpp"
#include "../domain/state_topics.hpp"
#include "../domain/text_edit_buffer.hpp"
#include "../domain/title_config_store.hpp"
#include "../domain/transform_date_entry.hpp"
#include "../infrastructure/graphics/pick_id.hpp"
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
#include "calendar/text_input_event.hpp"
#include "calendar/title_text_editor.hpp"
#include "event_bus.hpp"
#include "interaction_topics.hpp"
#include "state_burst.hpp"

namespace app_binder {
namespace {

// Hangs a consumer method onto a producer's signal for the wiring's lifetime.
//
// Most consumers are no QObjects — the stores, the rendering adapter, the text
// editor — so the connection cannot carry the receiver as its context. It
// carries the scope object instead: Qt releases every connection when that
// dies, and the receiver has to outlive it, which the composition root's member
// order settles. Signal and slot keep their own parameter types, so a topic
// publishing `const T&` also reaches a slot taking `T`.
template <typename Sender, typename SentValue, typename Receiver,
          typename ReceivedValue>
void Connect(QObject& scope, Sender& sender, void (Sender::*signal)(SentValue),
             Receiver& receiver, void (Receiver::*slot)(ReceivedValue)) {
  QObject::connect(&sender, signal, &scope, [&receiver, slot](SentValue value) {
    (receiver.*slot)(value);
  });
}

void BindDateEntries(QObject& scope, EventBus& bus, AppComponents& components) {
  // Panel -> store (user input)
  Connect(scope, components.data_table_panel,
          &DateTablePanel::DateEntriesEdited, components.date_entry_store,
          &DateEntryStore::ReceiveDateEntries);

  // Topic -> consumers. The store publishes itself.
  Connect(scope, bus.date_entries, &domain::DateEntriesTopic::Published,
          components.data_table_panel, &DateTablePanel::ReceiveDateEntries);
  Connect(scope, bus.date_entries, &domain::DateEntriesTopic::Published,
          components.transform_date_entry,
          &TransformDateEntry::ReceiveDateEntries);

  // The transform adapter publishes on its own topic. No shift: DatePeriod is
  // half-open [begin, end) everywhere, so the end is exclusive already. The
  // earlier {end_days = 1} was a correction out of the old inclusive model and
  // made every bar one day too long.
  Connect(scope, bus.transformed_date_entries,
          &domain::DateEntriesTopic::Published, components.calendar_page,
          &CalendarPage::ReceiveDateEntries);
}

void BindDateGroups(QObject& scope, EventBus& bus, AppComponents& components) {
  Connect(scope, components.date_groups_table_panel,
          &DateGroupsTablePanel::DateGroupsEdited, components.date_groups_store,
          &DateGroupStore::ReceiveDateGroups);

  Connect(scope, bus.date_groups, &domain::DateGroupsTopic::Published,
          components.date_groups_table_panel,
          &DateGroupsTablePanel::ReceiveDateGroups);
  Connect(scope, bus.date_groups, &domain::DateGroupsTopic::Published,
          components.date_entry_store, &DateEntryStore::ReceiveDateGroups);
  Connect(scope, bus.date_groups, &domain::DateGroupsTopic::Published,
          components.data_table_panel, &DateTablePanel::ReceiveDateGroups);
  // The store synthesises the per-group shape configurations out of the palette
  // and publishes them anew — that must run before the scene rebuild below,
  // which reads the updated configurations off the bus. What keeps the order is
  // the order of these two calls: "if a signal is connected to several slots,
  // the slots are activated in the same order as the order the connection was
  // made" (https://doc.qt.io/qt-6/qobject.html#connect).
  Connect(scope, bus.date_groups, &domain::DateGroupsTopic::Published,
          components.shape_configuration_store,
          &ShapeConfigurationStore::ReceiveDateGroups);
  Connect(scope, bus.date_groups, &domain::DateGroupsTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveDateGroups);
}

void BindPageSetup(QObject& scope, EventBus& bus, AppComponents& components) {
  Connect(scope, components.page_setup_panel, &PageSetupPanel::PageSetupEdited,
          components.page_setup_store, &PageSetupStore::ReceivePageSetup);

  Connect(scope, bus.page_setup, &domain::PageSetupTopic::Published,
          components.page_setup_panel, &PageSetupPanel::ReceivePageSetup);
  Connect(scope, bus.page_setup, &domain::PageSetupTopic::Published,
          components.calendar_page, &CalendarPage::ReceivePageSetup);
  Connect(scope, bus.page_setup, &domain::PageSetupTopic::Published,
          components.gl_canvas, &GLCanvas::ReceivePageSetup);
}

// The file path comes from the document, not from a store: loading and saving
// alone change it. The display is a pure consumer.
void BindProjectFilePath(QObject& scope, EventBus& bus,
                         AppComponents& components) {
  Connect(scope, bus.project_file_path, &domain::FilePathTopic::Published,
          components.document_setup_panel,
          &DocumentSetupPanel::ReceiveProjectFilePath);
}

// The font has no store — the panel value goes straight onto the topic and from
// there to the renderer. Should the font ever get saved with the project, a
// FontStore steps into the same place as with the other topics.
void BindFont(QObject& scope, EventBus& bus, AppComponents& components) {
  Connect(scope, components.font_panel, &FontPanel::FontConfigChosen,
          bus.font_config, &domain::FontConfigTopic::Publish);

  Connect(scope, bus.font_config, &domain::FontConfigTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveFont);
}

void BindTitleConfig(QObject& scope, EventBus& bus, AppComponents& components) {
  Connect(scope, components.title_setup_panel,
          &TitleSetupPanel::TitleConfigEdited, components.title_config_store,
          &TitleConfigStore::ReceiveTitleConfig);

  Connect(scope, bus.title_config, &domain::TitleConfigTopic::Published,
          components.title_setup_panel, &TitleSetupPanel::ReceiveTitleConfig);
  Connect(scope, bus.title_config, &domain::TitleConfigTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveTitleConfig);
}

// The bracket around a burst of changes. The rendering adapter is its only
// consumer: it holds its rebuild while the bracket stands (#36).
void BindStateBurst(QObject& scope, EventBus& bus, AppComponents& components) {
  Connect(scope, bus.state_burst, &domain::StateBurstTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveStateBurst);
}

void BindShapeConfiguration(QObject& scope, EventBus& bus,
                            AppComponents& components) {
  Connect(scope, components.shape_setup_panel,
          &ShapeSetupPanel::ShapeConfigSetEdited,
          components.shape_configuration_store,
          &ShapeConfigurationStore::ReceiveShapeConfigSet);

  Connect(scope, bus.shape_config_set, &domain::ShapeConfigSetTopic::Published,
          components.shape_setup_panel,
          &ShapeSetupPanel::ReceiveShapeConfigSet);
  Connect(scope, bus.shape_config_set, &domain::ShapeConfigSetTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveShapeConfigSet);
  Connect(scope, bus.shape_config_set, &domain::ShapeConfigSetTopic::Published,
          components.scene_tree_panel, &SceneTreePanel::ReceiveShapeConfigSet);
}

void BindCalendarConfig(QObject& scope, EventBus& bus,
                        AppComponents& components) {
  Connect(scope, components.calendar_setup_panel,
          &CalendarSetupPanel::CalendarConfigEdited,
          components.calendar_configuration_store,
          &CalendarConfigStore::ReceiveCalendarConfig);

  Connect(scope, bus.calendar_config, &domain::CalendarConfigTopic::Published,
          components.calendar_setup_panel,
          &CalendarSetupPanel::ReceiveCalendarConfig);
  Connect(scope, bus.calendar_config, &domain::CalendarConfigTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveCalendarConfig);
}

// The rendering adapter publishes the scene snapshots itself; the scene tree
// panel is the only consumer. Its selection goes back the other way over the
// bus to the highlight in the renderer.
void BindSceneSnapshot(QObject& scope, EventBus& bus,
                       AppComponents& components) {
  Connect(scope, bus.scene_snapshot, &domain::SceneSnapshotTopic::Published,
          components.scene_tree_panel, &SceneTreePanel::ReceiveSceneSnapshot);

  Connect(scope, components.scene_tree_panel,
          &SceneTreePanel::SelectedNodeChanged, bus.selected_node,
          &domain::NodePathTopic::Publish);
  Connect(scope, bus.selected_node, &domain::NodePathTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveSelectedNode);
  Connect(scope, bus.selected_node, &domain::NodePathTopic::Published,
          components.scene_tree_panel, &SceneTreePanel::ReceiveSelectedNode);
}

// Picking: the canvas reports pointer movements in page space, the controller
// tests them through the rendering adapter and publishes hover and selection
// itself. Click and double click go to the text editor first: while an edit is
// running it consumes them (set the cursor, select a word), otherwise the
// binder passes them on to the controller.
//
// These stay plain callbacks rather than signals: a pick source answers with a
// hit, and a Qt signal carries no return value.
void BindInteraction(QObject& scope, EventBus& bus, AppComponents& components) {
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

  Connect(scope, bus.hovered, &application::HoveredTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveHovered);
  Connect(scope, bus.edit_requested, &application::EditRequestTopic::Published,
          components.title_text_editor, &TitleTextEditor::Begin);
  Connect(scope, bus.text_edit, &domain::TextEditTopic::Published,
          components.calendar_page, &CalendarPage::ReceiveTextEdit);
}

}  // namespace

void Bind(QObject& scope, EventBus& bus, AppComponents& components) {
  BindDateEntries(scope, bus, components);
  BindDateGroups(scope, bus, components);
  BindPageSetup(scope, bus, components);
  BindProjectFilePath(scope, bus, components);
  BindFont(scope, bus, components);
  BindTitleConfig(scope, bus, components);
  BindStateBurst(scope, bus, components);
  BindShapeConfiguration(scope, bus, components);
  BindCalendarConfig(scope, bus, components);
  BindSceneSnapshot(scope, bus, components);
  BindInteraction(scope, bus, components);
}

void ReleaseCallbacks(AppComponents& components) {
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
}

void SendInitialValues(EventBus& bus, AppComponents& components) {
  // Five producers in a row, one rebuild at the end (#36).
  const application::StateBurst burst(bus.state_burst);
  components.shape_configuration_store.SendShapeConfigSet();
  components.date_groups_store.SendDefaultValues();
  components.page_setup_panel.SendDefaultValues();
  components.title_setup_panel.SendDefaultValues();
  components.calendar_configuration_store.SendCalendarConfig();
}

}  // namespace app_binder

AppWiring::AppWiring(EventBus& bus, AppComponents components)
    : components_(components) {
  app_binder::Bind(connection_scope_, bus, components_);
  app_binder::SendInitialValues(bus, components_);
}

AppWiring::~AppWiring() { app_binder::ReleaseCallbacks(components_); }
