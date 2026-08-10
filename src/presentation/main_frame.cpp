#include "main_frame.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <cstdint>
#include <string>

#include "../application/app_config.hpp"
#include "../domain/date_format.hpp"
#include "calendar_panel.hpp"
#include "date_panel.hpp"
#include "document_panel.hpp"
#include "font_panel.hpp"
#include "gl_canvas.hpp"
#include "groups_panel.hpp"
#include "license_panel.hpp"
#include "main_menu.hpp"
#include "make_owned.hpp"
#include "page_panel.hpp"
#include "scene_tree_panel.hpp"
#include "shape_panel.hpp"
#include "title_panel.hpp"
#include "window_screenshot.hpp"

MainFrame::MainFrame(QWidget* parent,
                     const application::MainFrameConfig& config,
                     LocaleDateFormatter& locale_date_formatter)
    : QMainWindow(parent, config.flags),
      locale_date_formatter_(locale_date_formatter),
      menu_(GLCanvas::kExportPngDpi) {
  setWindowTitle(QString::fromStdString(config.title));
  setObjectName(QString::fromStdString(config.object_name));
  move(config.position);
  resize(config.size);

  CreateLayout(config.maximize_on_start);
  InitMenu();
}

DateTablePanel& MainFrame::DataTable() { return *data_table_panel_; }

DateGroupsTablePanel& MainFrame::DateGroupsTable() {
  return *date_groups_table_panel_;
}

DocumentSetupPanel& MainFrame::DocumentSetup() {
  return *document_setup_panel_;
}

PageSetupPanel& MainFrame::PageSetup() { return *page_setup_panel_; }

TitleSetupPanel& MainFrame::TitleSetup() { return *title_setup_panel_; }

CalendarSetupPanel& MainFrame::CalendarSetup() {
  return *calendar_setup_panel_;
}

FontPanel& MainFrame::Font() { return *font_panel_; }

ShapeSetupPanel& MainFrame::ShapeSetup() { return *shape_setup_panel_; }

SceneTreePanel& MainFrame::SceneTree() { return *scene_tree_panel_; }

GLCanvas& MainFrame::Canvas() { return *gl_canvas_; }

bool MainFrame::SelectTab(const std::string& label) {
  const QString wanted = QString::fromStdString(label);
  for (int index = 0; index < tabs_->count(); ++index) {
    if (tabs_->tabText(index).compare(wanted, Qt::CaseInsensitive) == 0) {
      tabs_->setCurrentIndex(index);
      return true;
    }
  }
  return false;
}

void MainFrame::CloseAfter(std::int64_t milliseconds) {
  QTimer::singleShot(static_cast<int>(milliseconds), this,
                     [this]() { close(); });
}

bool MainFrame::SaveFrameScreenshot(const std::string& file_path) {
  const window_screenshot::Overlay overlay{
      .image = gl_canvas_->CaptureImage(),
      .origin = gl_canvas_->mapTo(this, QPoint(0, 0)),
      .size = gl_canvas_->size()};
  return window_screenshot::SaveWindowPng(*this, overlay, file_path);
}

void MainFrame::closeEvent(QCloseEvent* event) {
  emit Closing();
  QMainWindow::closeEvent(event);
}

void MainFrame::CreateLayout(bool maximize_on_start) {
  auto* splitter = MakeOwned<QSplitter>(Qt::Horizontal, this);

  auto* tabs = MakeOwned<QTabWidget>(splitter);
  tabs_ = tabs;
  CreatePanels(tabs);

  auto* gl_canvas = MakeOwned<GLCanvas>(splitter);
  gl_canvas_ = gl_canvas;

  splitter->addWidget(tabs);
  splitter->addWidget(gl_canvas);
  // Half and half, and it stays that way when the window grows. Without the
  // sizes the tab widget's size hint would claim nearly everything and leave
  // the page a sliver; the stretch factors alone act on resizing only.
  splitter->setSizes({kEvenSplit, kEvenSplit});
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 1);

  setCentralWidget(splitter);

  if (maximize_on_start) {
    showMaximized();
  }
}

void MainFrame::CreatePanels(QTabWidget* tabs) {
  auto* data_table_panel =
      MakeOwned<DateTablePanel>(tabs, locale_date_formatter_);
  data_table_panel_ = data_table_panel;
  auto* date_groups_table_panel = MakeOwned<DateGroupsTablePanel>(tabs);
  date_groups_table_panel_ = date_groups_table_panel;
  auto* calendar_setup_panel = MakeOwned<CalendarSetupPanel>(tabs);
  calendar_setup_panel_ = calendar_setup_panel;
  auto* scene_tree_panel = MakeOwned<SceneTreePanel>(tabs);
  scene_tree_panel_ = scene_tree_panel;
  auto* shape_setup_panel = MakeOwned<ShapeSetupPanel>(tabs);
  shape_setup_panel_ = shape_setup_panel;

  // Page, font and title share the tab "Document"; the collecting panel owns
  // the three children, and they get wired one by one through the pointers
  // below.
  auto* document_setup_panel = MakeOwned<DocumentSetupPanel>(tabs);
  document_setup_panel_ = document_setup_panel;
  page_setup_panel_ = document_setup_panel->GetPageSetupPanel();
  font_panel_ = document_setup_panel->GetFontPanel();
  title_setup_panel_ = document_setup_panel->GetTitleSetupPanel();

  tabs->addTab(date_groups_table_panel, "Categories");
  tabs->addTab(data_table_panel, "Entries");
  tabs->addTab(document_setup_panel, "Document");
  tabs->addTab(shape_setup_panel, "Shapes");
  tabs->addTab(calendar_setup_panel, "Timeframe");
  tabs->addTab(scene_tree_panel, "Scene");
}

void MainFrame::InitMenu() {
  menu_.AttachTo(*this);
  const MainMenuActions& actions = menu_.Actions();

  ConnectFileCommand(actions.open_xml, FileCommand::kOpenXml);
  ConnectFileCommand(actions.save_xml, FileCommand::kSaveXml);
  ConnectFileCommand(actions.save_as_xml, FileCommand::kSaveXmlAs);
  ConnectFileCommand(actions.import_csv, FileCommand::kImportCsv);
  ConnectFileCommand(actions.export_csv, FileCommand::kExportCsv);
  ConnectFileCommand(actions.export_png, FileCommand::kExportPng);

  connect(actions.quit, &QAction::triggered, this, [this]() { close(); });
  connect(actions.license_info, &QAction::triggered, this, [this]() {
    LicenseInformationDialog dialog(this);
    dialog.exec();
  });
}

void MainFrame::ConnectFileCommand(QAction* action, FileCommand command) {
  connect(action, &QAction::triggered, this,
          [this, command]() { emit FileCommandRequested(command); });
}
