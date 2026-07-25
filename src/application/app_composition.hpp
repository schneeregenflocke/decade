#ifndef APP_COMPOSITION_HPP
#define APP_COMPOSITION_HPP

#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/weakref.h>
#include <wx/window.h>

#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "../domain/date_format.hpp"
#include "../presentation/file_commands.hpp"
#include "../presentation/main_frame.hpp"
#include "../presentation/wx_owned.hpp"
#include "app_binder.hpp"
#include "app_config.hpp"
#include "calendar/calendar_page.hpp"
#include "calendar/interaction_controller.hpp"
#include "event_bus.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"
#include "startup_script.hpp"

namespace application {

// Composition Root: baut alle langlebigen Teile, besitzt sie und hält sie
// zusammen. Sie enthält selbst keine Fach- und keine Widget-Logik — sie
// entscheidet nur, was es gibt und wie lange.
//
// Zwei Teile entstehen erst später, weil OpenGL verzögert bereitsteht: der
// Rendering-Adapter und die Verdrahtung. Beide liegen in einem `optional` und
// werden beim Zerstören des Fensters wieder aufgelöst — solange Panels und
// Canvas noch leben. Ohne das liefe ein beim Beenden noch gefeuertes
// Panel-Ereignis in bereits zerstörte Objekte.
class AppComposition {
 public:
  AppComposition(LocaleDateFormatter& locale_date_formatter,
                 RuntimeOptions options)
      : runtime_options_(std::move(options)),
        document_(bus_, locale_date_formatter),
        interaction_controller_(bus_.hovered()),
        startup_script_(runtime_options_, document_) {
    auto* frame = MakeOwned<MainFrame>(nullptr, DefaultMainFrameConfig(),
                                       locale_date_formatter);
    frame_ = frame;

    file_commands_.emplace(*frame, document_);
    frame->SignalFileCommand().connect(&FileCommands::Execute,
                                       &file_commands_.value());
    frame->Bind(wxEVT_CLOSE_WINDOW, &AppComposition::OnFrameClose, this);
    frame->Bind(wxEVT_DESTROY, &AppComposition::OnFrameDestroy, this);

    startup_script_.RunBeforeGraphics(*frame);

    frame->Show();
    frame->Raise();
    frame->Canvas().InitOpenGL(
        [this]() { OnGraphicsReady(); },
        [this](const std::string& message) { OnGraphicsFailed(message); });
  }

  ~AppComposition() { ReleaseGraphics(); }
  AppComposition(const AppComposition&) = delete;
  AppComposition& operator=(const AppComposition&) = delete;
  AppComposition(AppComposition&&) = delete;
  AppComposition& operator=(AppComposition&&) = delete;

  [[nodiscard]] MainFrame& Frame() { return *frame_; }

 private:
  // Läuft, sobald der GL-Kontext steht — erst hier darf GL-Zustand angefasst
  // werden, und erst hier gibt es etwas zu verdrahten.
  void OnGraphicsReady() {
    CalendarPage& calendar_page = calendar_page_.emplace(
        frame_->Canvas(), frame_->Font().GetFontFilePath(),
        bus_.scene_snapshot());
    wiring_.emplace(bus_, Components(calendar_page));
    startup_script_.RunAfterGraphics(*frame_, calendar_page);
  }

  // Ohne Kontext läuft der Ready-Pfad nie: keine Verdrahtung, keine
  // Startwerte. Ein bedienbares Fenster in diesem Zustand stürzt beim ersten
  // Klick ab — deshalb melden und schliessen.
  //
  // Über CallAfter, weil der erste Paint noch vor der Event-Loop eintreffen
  // kann: ein Close() von dort verpufft, und ein modaler Dialog stünde vor der
  // Loop.
  void OnGraphicsFailed(const std::string& message) {
    std::cerr << message << '\n';
    frame_->CallAfter([this, message]() {
      if (!IsNonInteractiveRun(runtime_options_)) {
        wxMessageBox(message, "OpenGL", wxOK | wxICON_ERROR, frame_);
      }
      frame_->Close(true);
    });
  }

  [[nodiscard]] AppComponents Components(CalendarPage& calendar_page) {
    return AppComponents{
        .date_groups_store = document_.DateGroups(),
        .date_entry_store = document_.DateEntries(),
        .transform_date_entry = document_.Transform(),
        .page_setup_store = document_.PageSetup(),
        .title_config_store = document_.TitleConfiguration(),
        .shape_configuration_store = document_.ShapeConfiguration(),
        .calendar_configuration_store = document_.CalendarConfiguration(),
        .data_table_panel = frame_->DataTable(),
        .date_groups_table_panel = frame_->DateGroupsTable(),
        .page_setup_panel = frame_->PageSetup(),
        .title_setup_panel = frame_->TitleSetup(),
        .calendar_setup_panel = frame_->CalendarSetup(),
        .font_panel = frame_->Font(),
        .scene_tree_panel = frame_->SceneTree(),
        .calendar_page = calendar_page,
        .gl_canvas = frame_->Canvas(),
        .interaction_controller = interaction_controller_,
    };
  }

  void OnFrameClose(wxCloseEvent& event) {
    ReleaseGraphics();
    event.Skip();
  }

  // Zweiter Ausgang: ein Fenster kann auch ohne Close-Ereignis sterben. Der
  // Destroy-Event kommt, bevor der Basisdestruktor die Kinder abräumt.
  void OnFrameDestroy(wxWindowDestroyEvent& event) {
    if (event.GetEventObject() == frame_.get()) {
      ReleaseGraphics();
    }
    event.Skip();
  }

  // Idempotent: löst die Verdrahtung, solange beide Enden noch leben.
  void ReleaseGraphics() {
    wiring_.reset();
    calendar_page_.reset();
    file_commands_.reset();
  }

  // Zuerst deklariert, also zuletzt zerstört: jeder Produzent veröffentlicht
  // über den Bus, und beim Abräumen kann noch etwas feuern.
  EventBus bus_;

  RuntimeOptions runtime_options_;
  ProjectDocument document_;
  InteractionController interaction_controller_;
  StartupScript startup_script_;

  wxWeakRef<MainFrame> frame_;
  std::optional<FileCommands> file_commands_;
  std::optional<CalendarPage> calendar_page_;
  std::optional<AppWiring> wiring_;
};

}  // namespace application

#endif  // APP_COMPOSITION_HPP
