#ifndef MAIN_FRAME_HPP
#define MAIN_FRAME_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <cstdint>
#include <sigslot/signal.hpp>
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

// Menu commands handled outside the window, because they concern the project.
// Quit and the licence text stay here — they need nobody but the window
// itself.
enum class FileCommand : std::uint8_t {
  kOpenXml,
  kSaveXml,
  kSaveXmlAs,
  kImportCsv,
  kExportCsv,
  kExportPng,
};

// The main window: it builds the layout, owns panels, canvas and menu and
// reports menu commands as a signal. It knows neither stores nor bus — whoever
// wires the panels fetches them through the accessors.
class MainFrame : public QMainWindow {
 public:
  MainFrame(QWidget* parent, const application::MainFrameConfig& config,
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

  ~MainFrame() override = default;
  MainFrame(const MainFrame&) = delete;
  MainFrame& operator=(const MainFrame&) = delete;
  MainFrame(MainFrame&&) = delete;
  MainFrame& operator=(MainFrame&&) = delete;

  [[nodiscard]] auto& SignalFileCommand() { return signal_file_command_; }

  // Fires while the window is still whole, so whoever holds wiring onto its
  // children can dissolve it before the children go.
  [[nodiscard]] auto& SignalClosing() { return signal_closing_; }

  [[nodiscard]] DateTablePanel& DataTable() { return *data_table_panel_; }
  [[nodiscard]] DateGroupsTablePanel& DateGroupsTable() {
    return *date_groups_table_panel_;
  }
  [[nodiscard]] DocumentSetupPanel& DocumentSetup() {
    return *document_setup_panel_;
  }
  [[nodiscard]] PageSetupPanel& PageSetup() { return *page_setup_panel_; }
  [[nodiscard]] TitleSetupPanel& TitleSetup() { return *title_setup_panel_; }
  [[nodiscard]] CalendarSetupPanel& CalendarSetup() {
    return *calendar_setup_panel_;
  }
  [[nodiscard]] FontPanel& Font() { return *font_panel_; }
  [[nodiscard]] ShapeSetupPanel& ShapeSetup() { return *shape_setup_panel_; }
  [[nodiscard]] SceneTreePanel& SceneTree() { return *scene_tree_panel_; }
  [[nodiscard]] GLCanvas& Canvas() { return *gl_canvas_; }

  // Preselects the tab with this caption (case-insensitively). It reports
  // whether the tab exists.
  [[nodiscard]] bool SelectTab(const std::string& label) {
    const QString wanted = QString::fromStdString(label);
    for (int index = 0; index < tabs_->count(); ++index) {
      if (tabs_->tabText(index).compare(wanted, Qt::CaseInsensitive) == 0) {
        tabs_->setCurrentIndex(index);
        return true;
      }
    }
    return false;
  }

  // Closes the window after N milliseconds — for headless runs.
  void CloseAfter(std::int64_t milliseconds) {
    QTimer::singleShot(static_cast<int>(milliseconds), this,
                       [this]() { close(); });
  }

  // Writes the whole window as a PNG: the widget capture plus the mounted-in GL
  // content, which the widget capture does not draw.
  [[nodiscard]] bool SaveFrameScreenshot(const std::string& file_path) {
    const window_screenshot::Overlay overlay{
        .image = gl_canvas_->CaptureImage(),
        .origin = gl_canvas_->mapTo(this, QPoint(0, 0)),
        .size = gl_canvas_->size()};
    return window_screenshot::SaveWindowPng(*this, overlay, file_path);
  }

 protected:
  void closeEvent(QCloseEvent* event) override {
    signal_closing_();
    QMainWindow::closeEvent(event);
  }

 private:
  // Any two equal numbers do: QSplitter reads the sizes as proportions of the
  // space it actually has.
  static constexpr int kEvenSplit = 10000;

  void CreateLayout(bool maximize_on_start) {
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

  void CreatePanels(QTabWidget* tabs) {
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

  void InitMenu() {
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

  void ConnectFileCommand(QAction* action, FileCommand command) {
    connect(action, &QAction::triggered, this,
            [this, command]() { signal_file_command_(command); });
  }

  LocaleDateFormatter& locale_date_formatter_;

  QPointer<QTabWidget> tabs_;

  QPointer<DateGroupsTablePanel> date_groups_table_panel_;
  QPointer<DocumentSetupPanel> document_setup_panel_;
  QPointer<PageSetupPanel> page_setup_panel_;
  QPointer<TitleSetupPanel> title_setup_panel_;
  QPointer<CalendarSetupPanel> calendar_setup_panel_;
  QPointer<GLCanvas> gl_canvas_;
  QPointer<FontPanel> font_panel_;
  QPointer<DateTablePanel> data_table_panel_;
  QPointer<SceneTreePanel> scene_tree_panel_;
  QPointer<ShapeSetupPanel> shape_setup_panel_;

  MainMenu menu_;
  sigslot::signal<FileCommand> signal_file_command_;
  sigslot::signal<> signal_closing_;
};

#endif  // MAIN_FRAME_HPP
