#include "app_composition.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "../domain/date_format.hpp"
#include "../presentation/file_commands.hpp"
#include "../presentation/gl_canvas.hpp"
#include "../presentation/main_frame.hpp"
#include "app_binder.hpp"
#include "app_config.hpp"
#include "calendar/calendar_page.hpp"
#include "event_bus.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"
#include "startup_script.hpp"

namespace application {

AppComposition::AppComposition(LocaleDateFormatter& locale_date_formatter,
                               RuntimeOptions options)
    : runtime_options_(std::move(options)),
      document_(bus_, locale_date_formatter),
      interaction_controller_(bus_.hovered(), bus_.selected_node(),
                              bus_.edit_requested()),
      title_text_editor_(document_.TitleConfiguration(), bus_.text_edit()),
      startup_script_(runtime_options_, document_),
      // A top-level window has no Qt parent to own it, so the composition
      // root does — the same hand that owns everything else here.
      frame_(std::make_unique<MainFrame>(nullptr, DefaultMainFrameConfig(),
                                         locale_date_formatter)) {
  file_commands_.emplace(*frame_, document_);
  frame_->SignalFileCommand().connect(&FileCommands::Execute,
                                      &file_commands_.value());
  frame_->SignalClosing().connect(&AppComposition::ReleaseGraphics, this);

  startup_script_.RunBeforeGraphics(*frame_);

  frame_->show();
  frame_->raise();
  frame_->Canvas().InitOpenGL(
      [this]() { OnGraphicsReady(); },
      [this](const std::string& message) { OnGraphicsFailed(message); });
}

AppComposition::~AppComposition() { ReleaseGraphics(); }

MainFrame& AppComposition::Frame() { return *frame_; }

void AppComposition::OnGraphicsReady() {
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

void AppComposition::OnGraphicsFailed(const std::string& message) {
  std::cerr << message << '\n';
  const bool interactive = !IsNonInteractiveRun(runtime_options_);
  QTimer::singleShot(0, frame_.get(), [this, message, interactive]() {
    if (interactive) {
      QMessageBox::critical(frame_.get(), "OpenGL",
                            QString::fromStdString(message));
    }
    frame_->close();
  });
}

AppComponents AppComposition::Components(CalendarPage& calendar_page) {
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

void AppComposition::ReleaseGraphics() {
  wiring_.reset();
  if (calendar_page_.has_value() && frame_ && frame_->Canvas().HasEngine()) {
    frame_->Canvas().MakeGraphicsCurrent();
  }
  calendar_page_.reset();
  file_commands_.reset();
}

}  // namespace application
