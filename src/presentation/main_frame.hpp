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
  Q_OBJECT

 public:
  MainFrame(QWidget* parent, const application::MainFrameConfig& config,
            LocaleDateFormatter& locale_date_formatter);

  ~MainFrame() override = default;
  MainFrame(const MainFrame&) = delete;
  MainFrame& operator=(const MainFrame&) = delete;
  MainFrame(MainFrame&&) = delete;
  MainFrame& operator=(MainFrame&&) = delete;

  [[nodiscard]] DateTablePanel& DataTable();
  [[nodiscard]] DateGroupsTablePanel& DateGroupsTable();
  [[nodiscard]] DocumentSetupPanel& DocumentSetup();
  [[nodiscard]] PageSetupPanel& PageSetup();
  [[nodiscard]] TitleSetupPanel& TitleSetup();
  [[nodiscard]] CalendarSetupPanel& CalendarSetup();
  [[nodiscard]] FontPanel& Font();
  [[nodiscard]] ShapeSetupPanel& ShapeSetup();
  [[nodiscard]] SceneTreePanel& SceneTree();
  [[nodiscard]] GLCanvas& Canvas();

  // Preselects the tab with this caption (case-insensitively). It reports
  // whether the tab exists.
  [[nodiscard]] bool SelectTab(const std::string& label);

  // Closes the window after N milliseconds — for headless runs.
  void CloseAfter(std::int64_t milliseconds);

  // Writes the whole window as a PNG: the widget capture plus the mounted-in GL
  // content, which the widget capture does not draw.
  [[nodiscard]] bool SaveFrameScreenshot(const std::string& file_path);

 signals:
  void FileCommandRequested(FileCommand command);

  // Fires while the window is still whole, so whoever holds wiring onto its
  // children can dissolve it before the children go.
  void Closing();

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  // Any two equal numbers do: QSplitter reads the sizes as proportions of the
  // space it actually has.
  static constexpr int kEvenSplit = 10000;

  void CreateLayout(bool maximize_on_start);

  void CreatePanels(QTabWidget* tabs);

  void InitMenu();

  void ConnectFileCommand(QAction* action, FileCommand command);

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
};

#endif  // MAIN_FRAME_HPP
