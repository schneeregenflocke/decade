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
#include "../presentation/main_window.hpp"
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
      interaction_controller_(bus_.hovered, bus_.selected_node,
                              bus_.edit_requested),
      title_text_editor_(document_.TitleConfiguration(), bus_.text_edit),
      startup_script_(runtime_options_, document_),
      // A top-level window has no Qt parent to own it, so the composition
      // root does — the same hand that owns everything else here.
      window_(std::make_unique<MainWindow>(nullptr, DefaultMainWindowConfig(),
                                           locale_date_formatter)) {
  file_commands_.emplace(*window_, document_);
  // The commands go once the graphics are released, while the window can still
  // raise a menu action — hence the check rather than a captured address.
  QObject::connect(window_.get(), &MainWindow::FileCommandRequested,
                   &connection_scope_, [this](FileCommand command) {
                     if (file_commands_.has_value()) {
                       file_commands_->Execute(command);
                     }
                   });
  QObject::connect(window_.get(), &MainWindow::Closing, &connection_scope_,
                   [this]() { ReleaseGraphics(); });

  startup_script_.RunBeforeGraphics(*window_);

  window_->show();
  window_->raise();
  window_->Canvas().InitOpenGL(
      [this]() { OnGraphicsReady(); },
      [this](const std::string& message) { OnGraphicsFailed(message); });
}

AppComposition::~AppComposition() { ReleaseGraphics(); }

void AppComposition::OnGraphicsReady() {
  try {
    CalendarPage& calendar_page =
        calendar_page_.emplace(window_->Canvas().Engine(), window_->Canvas(),
                               window_->Font().GetFontConfig(),
                               bus_.scene_snapshot, bus_.calendar_metrics);
    wiring_.emplace(bus_, Components(calendar_page));
    startup_script_.RunAfterGraphics(*window_, calendar_page,
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
  QTimer::singleShot(0, window_.get(), [this, message, interactive]() {
    if (interactive) {
      QMessageBox::critical(window_.get(), "OpenGL",
                            QString::fromStdString(message));
    }
    window_->close();
  });
}

AppComponents AppComposition::Components(CalendarPage& calendar_page) {
  return AppComponents{
      .date_categories_store = document_.DateCategories(),
      .date_entry_store = document_.DateEntries(),
      .page_setup_store = document_.PageSetup(),
      .title_config_store = document_.TitleConfiguration(),
      .shape_configuration_store = document_.ShapeConfiguration(),
      .calendar_configuration_store = document_.CalendarConfiguration(),
      .project_document = document_,
      .data_table_panel = window_->DataTable(),
      .date_categories_table_panel = window_->DateCategoriesTable(),
      .document_setup_panel = window_->DocumentSetup(),
      .page_setup_panel = window_->PageSetup(),
      .title_setup_panel = window_->TitleSetup(),
      .calendar_setup_panel = window_->CalendarSetup(),
      .csv_import_panel = window_->CsvImport(),
      .font_panel = window_->Font(),
      .scene_tree_panel = window_->SceneTree(),
      .shape_setup_panel = window_->ShapeSetup(),
      .calendar_page = calendar_page,
      .gl_canvas = window_->Canvas(),
      .interaction_controller = interaction_controller_,
      .title_text_editor = title_text_editor_,
  };
}

void AppComposition::ReleaseGraphics() {
  wiring_.reset();
  if (calendar_page_.has_value() && window_ && window_->Canvas().HasEngine()) {
    window_->Canvas().MakeGraphicsCurrent();
  }
  calendar_page_.reset();
  file_commands_.reset();
}

}  // namespace application
