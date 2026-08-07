#ifndef APP_COMPOSITION_HPP
#define APP_COMPOSITION_HPP

#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/weakref.h>
#include <wx/window.h>

#include <exception>
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
#include "calendar/title_text_editor.hpp"
#include "event_bus.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"
#include "startup_script.hpp"

namespace application {

// The composition root: it builds every long-lived part, owns them and holds
// them together. It carries neither domain nor widget logic itself — it decides
// what exists and for how long, and nothing more.
//
// Two parts come into being later, because OpenGL stands ready with a delay:
// the rendering adapter and the wiring. Both sit in an `optional` and get
// dissolved again when the window is destroyed — while panels and canvas are
// still alive. Without that, a panel event still firing on shutdown would run
// into objects already destroyed.
class AppComposition {
 public:
  AppComposition(LocaleDateFormatter& locale_date_formatter,
                 RuntimeOptions options)
      : runtime_options_(std::move(options)),
        document_(bus_, locale_date_formatter),
        interaction_controller_(bus_.hovered(), bus_.selected_node(),
                                bus_.edit_requested()),
        title_text_editor_(document_.TitleConfiguration(), bus_.text_edit()),
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
  // Runs as soon as the GL context stands — only here may GL state be touched,
  // and only here is there anything to wire.
  //
  // A scene that cannot be built lands in the same place as a context that
  // never came up: the window would stand there operable and die on the first
  // interaction. Building it needs GL resources that exist by then or not at
  // all — a missing shader, for instance — so the failure is final and gets
  // reported rather than retried. Whatever came into being before the throw
  // gets dissolved first, so no half-built page survives.
  void OnGraphicsReady() {
    try {
      CalendarPage& calendar_page = calendar_page_.emplace(
          frame_->Canvas().Engine(), frame_->Canvas(),
          frame_->Font().GetFontConfig(), bus_.scene_snapshot());
      wiring_.emplace(bus_, Components(calendar_page));
      startup_script_.RunAfterGraphics(*frame_, calendar_page,
                                       title_text_editor_);
    } catch (const std::exception& error) {
      wiring_.reset();
      calendar_page_.reset();
      OnGraphicsFailed(std::string("scene setup failed: ") + error.what());
    }
  }

  // Without a context the ready path never runs: no wiring, no initial values.
  // An operable window in that state crashes on the first click — hence report
  // and close.
  //
  // Through CallAfter, because the first paint can arrive before the event
  // loop: a Close() from there fizzles out, and a modal dialogue would stand in
  // front of the loop.
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
        .document_setup_panel = frame_->DocumentSetup(),
        .page_setup_panel = frame_->PageSetup(),
        .title_setup_panel = frame_->TitleSetup(),
        .calendar_setup_panel = frame_->CalendarSetup(),
        .font_panel = frame_->Font(),
        .scene_tree_panel = frame_->SceneTree(),
        .shape_setup_panel = frame_->ShapeSetup(),
        .calendar_page = calendar_page,
        .gl_canvas = frame_->Canvas(),
        .interaction_controller = interaction_controller_,
        .title_text_editor = title_text_editor_,
    };
  }

  void OnFrameClose(wxCloseEvent& event) {
    ReleaseGraphics();
    event.Skip();
  }

  // A second exit: a window can die without a close event too. The destroy
  // event arrives before the base destructor clears the children away.
  void OnFrameDestroy(wxWindowDestroyEvent& event) {
    if (event.GetEventObject() == frame_.get()) {
      ReleaseGraphics();
    }
    event.Skip();
  }

  // Idempotent: it dissolves the wiring while both ends are still alive.
  void ReleaseGraphics() {
    wiring_.reset();
    calendar_page_.reset();
    file_commands_.reset();
  }

  // Declared first, so destroyed last: every producer publishes over the bus,
  // and something can still fire while clearing away.
  EventBus bus_;

  RuntimeOptions runtime_options_;
  ProjectDocument document_;
  InteractionController interaction_controller_;
  TitleTextEditor title_text_editor_;
  StartupScript startup_script_;

  wxWeakRef<MainFrame> frame_;
  std::optional<FileCommands> file_commands_;
  std::optional<CalendarPage> calendar_page_;
  std::optional<AppWiring> wiring_;
};

}  // namespace application

#endif  // APP_COMPOSITION_HPP
