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

// Die eine Stelle, an der Stores, Panels, Rendering-Adapter und GL-Canvas
// zusammenkommen.
//
// Zwei Richtungen, zwei Regeln:
//   * Ein Panel-Edit ist ein *Befehl* und geht direkt an den besitzenden Store.
//     Dessen `Receive*` ist damit der einzige Ort, an dem der kanonische
//     Zustand entsteht.
//   * Der neue Zustand ist eine *Tatsache* und geht über den Bus: der Store
//     veröffentlicht auf seinem Topic, jeder Konsument hängt sich dort an.
// Würden Panels ihre Edits auf dasselbe Topic legen, das sie abonnieren, gäbe
// es Rückkopplungen; die Trennung verhindert das und hält Produzent und
// Konsument voneinander unabhängig.
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

// Hängt ein Panel-Signal an ein Bus-Topic. Nötig nur dort, wo der Produzent
// in der Presentation sitzt und deshalb kein Topic eingesetzt bekommt.
template <typename Signal, typename Topic>
void Forward(Signal& from, Topic& to) {
  from.connect([&to](const auto& value) { to(value); });
}

inline void BindDateEntries(EventBus& bus, AppComponents& components) {
  // Panel -> Store (Nutzereingabe)
  components.data_table_panel.SignalTableDateEntries().connect(
      &DateEntryStore::ReceiveDateEntries, &components.date_entry_store);

  // Topic -> Konsumenten. Der Store veröffentlicht selbst.
  bus.date_entries().connect(&DateTablePanel::ReceiveDateEntries,
                             &components.data_table_panel);
  bus.date_entries().connect(&TransformDateEntry::ReceiveDateEntries,
                             &components.transform_date_entry);

  // Der Transform-Adapter veröffentlicht auf dem eigenen Topic. Keine
  // Verschiebung: DatePeriod ist überall halb-offen [begin, end), das Ende
  // also bereits exklusiv. Das frühere {end_days = 1} war eine Korrektur aus
  // dem alten inklusiven Modell und machte jeden Balken einen Tag zu lang.
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
  // Der Store synthetisiert die gruppenweisen Shape-Konfigurationen aus der
  // Palette und veröffentlicht sie neu — das muss vor dem Neuaufbau der Szene
  // laufen, der die aktualisierten Konfigurationen vom Bus liest.
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

// Der Dateipfad kommt vom Dokument, nicht von einem Store: nur Laden und
// Speichern ändern ihn. Die Anzeige ist reiner Konsument.
inline void BindProjectFilePath(EventBus& bus, AppComponents& components) {
  bus.project_file_path().connect(&DocumentSetupPanel::ReceiveProjectFilePath,
                                  &components.document_setup_panel);
}

// Die Schrift hat keinen Store — der Panelwert geht direkt aufs Topic und von
// dort an den Renderer. Soll die Schrift einmal mit dem Projekt gespeichert
// werden, tritt ein FontStore an dieselbe Stelle wie bei den anderen Themen.
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

// Der Rendering-Adapter veröffentlicht die Szenen-Momentaufnahmen selbst; das
// Szenenbaum-Panel ist der einzige Konsument. Dessen Auswahl geht umgekehrt
// über den Bus zurück an die Hervorhebung im Renderer.
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

// Picking: das Canvas meldet Zeigerbewegungen im Seitenraum, der Controller
// testet sie über den Rendering-Adapter und veröffentlicht Hover und Auswahl
// selbst. Klick und Doppelklick gehen zuerst an den Texteditor: läuft eine
// Bearbeitung, verbraucht er sie (Cursor setzen, Wort wählen), sonst reicht der
// Binder sie an den Controller weiter.
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

// Gegenstück zu Bind: trennt alle Verbindungen. Nötig beim Beenden — die
// wx-Children (Panels, GL-Canvas) sterben erst im ~wxFrame-Basisdestruktor,
// also nach den Stores und dem EventBus. Feuert ein Control beim Zerstören noch
// ein Ereignis (ein offener Editor-Commit etwa), liefe der Slot sonst in
// bereits zerstörte Objekte.
inline void Unbind(EventBus& bus, AppComponents& components) {
  // Ereignisquellen der Presentation-Schicht.
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

  // Bus-Topics: ihre Slots zeigen auf Panels und den Rendering-Adapter. Die
  // Produzenten dürfen danach weiter veröffentlichen — es hört nur niemand zu.
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

// Die Verdrahtung als Lebensdauer statt als zwei von Hand gepaarte Aufrufe:
// verbinden beim Bauen, trennen beim Zerstören. Als letztes Mitglied deklariert
// stirbt sie vor Stores und Bus — genau die Reihenfolge, die das Trennen
// braucht.
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
