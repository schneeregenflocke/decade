#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <wx/defs.h>
#include <wx/event.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/weakref.h>
#include <wx/window.h>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "../gui/calendar_panel.hpp"
#include "../gui/date_panel.hpp"
#include "../gui/font_panel.hpp"
#include "../gui/groups_panel.hpp"
#include "../gui/license_panel.hpp"
#include "../gui/opengl_panel.hpp"
#include "../gui/page_panel.hpp"
#include "../gui/shape_panel.hpp"
#include "../gui/title_panel.hpp"
#include "../packages/calendar_config.hpp"
#include "../packages/date_store.hpp"
#include "../packages/group_store.hpp"
#include "../packages/page_config.hpp"
#include "../packages/shape_config.hpp"
#include "../packages/title_config.hpp"
#include "binding/calendar_page.hpp"
#include "binding/event_bus.hpp"
#include "binding/main_window_binder.hpp"
#include "services/project_io.hpp"

class MainWindow : public wxFrame {
 public:
  MainWindow(wxWindow* parent, const wxString& title, const wxPoint& pos,
             const wxSize& size, bool maximize_on_start = true);
  ~MainWindow() override;
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;
  MainWindow(MainWindow&&) = delete;
  MainWindow& operator=(MainWindow&&) = delete;

 private:
  void CreateLayout(bool maximize_on_start);
  void CreatePanels(wxNotebook* notebook);
  void InitializeOpenGL();
  void EstablishConnections();
  void LoadDefaultDates();
  void ConfigureAutoExitTimer();
  void DumpPngIfRequested();
  void InitMenu();

  void CallbackLoadXML(wxCommandEvent& event);
  void CallbackSaveXML(wxCommandEvent& event);
  void CallbackImportCSV(wxCommandEvent& event);
  void CallbackExportCSV(wxCommandEvent& event);
  void CallbackExportPNG(wxCommandEvent& event);
  void CallbackExit(wxCommandEvent& event);
  void OnExitTimer(wxTimerEvent& event);
  void CallbackLicenseInfo(wxCommandEvent& event);

  void LoadXML(const std::string& filepath);
  void SaveXML(const std::string& filepath);

  struct Impl;
  std::unique_ptr<Impl> impl_;

  std::string xml_file_path;
  wxTimer exit_timer;

  const int id_save_xml;
  const int id_save_as_xml;
};

namespace main_window_detail {
constexpr int kOpenGLMajor = 4;
constexpr int kOpenGLMinor = 6;
}  // namespace main_window_detail

struct MainWindow::Impl {
  // Declared first so it is destroyed *last* — every producer (stores,
  // panels, the GL canvas) emits via `event_bus`, and some emissions can run
  // during teardown. Keeping the bus alive longer than its capturing lambdas
  // avoids dangling references in slot callbacks.
  EventBus event_bus;

  wxWeakRef<wxSplitterWindow> main_splitter;
  wxWeakRef<wxNotebook> notebook;

  wxWeakRef<DateGroupsTablePanel> date_groups_table_panel;
  wxWeakRef<ElementsSetupsPanel> elements_setup_panel;
  wxWeakRef<PageSetupPanel> page_setup_panel;
  wxWeakRef<TitleSetupPanel> title_setup_panel;
  wxWeakRef<CalendarSetupPanel> calendar_setup_panel;
  wxWeakRef<GLCanvas> gl_canvas;
  wxWeakRef<FontPanel> font_panel;
  wxWeakRef<DateTablePanel> data_table_panel;

  DateGroupStore date_groups_store;
  DateIntervalBundleStore date_interval_bundle_store;
  TransformDateIntervalBundle transform_date_interval_bundle;
  PageSetupStore page_setup_store;
  TitleConfigStore title_config_store;
  ShapeConfigurationStorage shape_configuration_storage;
  CalendarConfigStorage calendar_configuration_storage;
  std::unique_ptr<CalendarPage> calendar_page;
};

inline MainWindow::MainWindow(wxWindow* parent, const wxString& title,
                              const wxPoint& pos, const wxSize& size,
                              bool maximize_on_start)
    : wxFrame(parent, wxID_ANY, title, pos, size),
      impl_(std::make_unique<Impl>()),
      exit_timer(this),
      id_save_xml(wxWindow::NewControlId()),
      id_save_as_xml(wxWindow::NewControlId()) {
  CreateLayout(maximize_on_start);
  InitMenu();
  InitializeOpenGL();
  ConfigureAutoExitTimer();
}

inline MainWindow::~MainWindow() = default;

inline void MainWindow::CreateLayout(bool maximize_on_start) {
  if (maximize_on_start) {
    Maximize();
  }

  auto main_splitter = std::make_unique<wxSplitterWindow>(
      this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
      wxSP_3D | wxSP_LIVE_UPDATE);
  impl_->main_splitter = main_splitter.get();
  auto* main_splitter_ptr = main_splitter.release();

  constexpr int minimum_pane_size = 5;
  constexpr double sash_gravity = 0.5;
  main_splitter_ptr->SetMinimumPaneSize(minimum_pane_size);
  main_splitter_ptr->SetSashGravity(sash_gravity);

  constexpr int kSizerBorder = 5;
  wxSizerFlags sizer_flags;
  sizer_flags.Proportion(1).Expand().Border(wxALL, kSizerBorder);

  auto notebook_panel = std::make_unique<wxPanel>(main_splitter_ptr, wxID_ANY);
  auto* notebook_panel_ptr = notebook_panel.release();
  auto notebook_panel_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
  auto* notebook_panel_sizer_ptr = notebook_panel_sizer.release();
  notebook_panel_ptr->SetSizer(notebook_panel_sizer_ptr);

  auto notebook = std::make_unique<wxNotebook>(
      notebook_panel_ptr, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
  impl_->notebook = notebook.get();
  auto* notebook_ptr = notebook.release();
  notebook_panel_sizer_ptr->Add(notebook_ptr, sizer_flags);

  CreatePanels(notebook_ptr);
  app::io::PrintRuntimeInfo(std::cout);

  auto gl_canvas_panel = std::make_unique<wxPanel>(main_splitter_ptr, wxID_ANY);
  auto* gl_canvas_panel_ptr = gl_canvas_panel.release();
  auto gl_canvas = std::make_unique<GLCanvas>(gl_canvas_panel_ptr);
  impl_->gl_canvas = gl_canvas.get();
  auto* gl_canvas_ptr = gl_canvas.release();
  auto gl_canvas_panel_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
  auto* gl_canvas_panel_sizer_ptr = gl_canvas_panel_sizer.release();
  gl_canvas_panel_ptr->SetSizer(gl_canvas_panel_sizer_ptr);
  gl_canvas_panel_sizer_ptr->Add(gl_canvas_ptr, sizer_flags);

  main_splitter_ptr->SplitVertically(notebook_panel_ptr, gl_canvas_panel_ptr);
}

inline void MainWindow::CreatePanels(wxNotebook* notebook) {
  impl_->data_table_panel =
      std::make_unique<DateTablePanel>(notebook).release();
  impl_->date_groups_table_panel =
      std::make_unique<DateGroupsTablePanel>(notebook).release();
  impl_->calendar_setup_panel =
      std::make_unique<CalendarSetupPanel>(notebook).release();
  impl_->elements_setup_panel =
      std::make_unique<ElementsSetupsPanel>(notebook).release();
  impl_->page_setup_panel =
      std::make_unique<PageSetupPanel>(notebook).release();
  impl_->font_panel = std::make_unique<FontPanel>(notebook).release();
  impl_->title_setup_panel =
      std::make_unique<TitleSetupPanel>(notebook).release();

  notebook->AddPage(impl_->data_table_panel, "Dates");
  notebook->AddPage(impl_->date_groups_table_panel, "Groups");
  notebook->AddPage(impl_->calendar_setup_panel, "Calendar");
  notebook->AddPage(impl_->elements_setup_panel, "Shapes");
  notebook->AddPage(impl_->page_setup_panel, "Page");
  notebook->AddPage(impl_->font_panel, "Font");
  notebook->AddPage(impl_->title_setup_panel, "Title");
}

inline void MainWindow::InitializeOpenGL() {
  CreateStatusBar(1);
  Show();
  Raise();

  const std::array<int, 2> gl_version{main_window_detail::kOpenGLMajor,
                                      main_window_detail::kOpenGLMinor};
  impl_->gl_canvas->InitOpenGL(gl_version, [this]() {
    impl_->calendar_page = std::make_unique<CalendarPage>(
        impl_->gl_canvas.get(), impl_->font_panel->GetFontFilePath());
    EstablishConnections();
    LoadDefaultDates();
    DumpPngIfRequested();
  });
}

inline void MainWindow::LoadDefaultDates() {
  // Auto-load a sample data set on every start so the calendar is populated
  // without a manual CSV import.
  //   DECADE_NO_DEFAULT_CSV=1     disables the auto-load entirely.
  //   DECADE_DEFAULT_CSV=<path>   overrides the file that gets loaded.
  // The default path is resolved relative to the working directory (the repo
  // root for typical runs).
  if (wxGetEnv("DECADE_NO_DEFAULT_CSV", nullptr)) {
    std::cout << "LoadDefaultDates: disabled via DECADE_NO_DEFAULT_CSV\n";
    return;
  }

  wxString override_path;
  const std::string default_csv =
      wxGetEnv("DECADE_DEFAULT_CSV", &override_path)
          ? override_path.ToStdString()
          : std::string("test-files/test_dates_1.csv");

  if (!wxFileExists(default_csv)) {
    std::cout << "LoadDefaultDates: " << default_csv
              << " not found, skipping\n";
    return;
  }
  impl_->date_interval_bundle_store.ReceiveDateIntervalBundles(
      app::io::ReadDateIntervalBundlesFromCsv(default_csv));
}

inline void MainWindow::DumpPngIfRequested() {
  wxString png_path;
  if (wxGetEnv("DECADE_DUMP_PNG", &png_path)) {
    const auto path = png_path.ToStdString();
    std::cout << "DECADE_DUMP_PNG: writing " << path << '\n';
    impl_->gl_canvas->SavePNG(path);
  }
  wxString window_png_path;
  if (wxGetEnv("DECADE_DUMP_WINDOW_PNG", &window_png_path)) {
    const auto path = window_png_path.ToStdString();
    // Defer until after the first real paint so the back buffer is populated.
    CallAfter([this, path]() {
      std::cout << "DECADE_DUMP_WINDOW_PNG: writing " << path << '\n';
      impl_->gl_canvas->SaveWindowPNG(path);
    });
  }
}

inline void MainWindow::EstablishConnections() {
  MainWindowComponents components{
      .date_groups_store = impl_->date_groups_store,
      .date_interval_bundle_store = impl_->date_interval_bundle_store,
      .transform_date_interval_bundle = impl_->transform_date_interval_bundle,
      .page_setup_store = impl_->page_setup_store,
      .title_config_store = impl_->title_config_store,
      .shape_configuration_storage = impl_->shape_configuration_storage,
      .calendar_configuration_storage = impl_->calendar_configuration_storage,
      .data_table_panel = *impl_->data_table_panel,
      .date_groups_table_panel = *impl_->date_groups_table_panel,
      .elements_setup_panel = *impl_->elements_setup_panel,
      .page_setup_panel = *impl_->page_setup_panel,
      .title_setup_panel = *impl_->title_setup_panel,
      .calendar_setup_panel = *impl_->calendar_setup_panel,
      .font_panel = *impl_->font_panel,
      .calendar_page = *impl_->calendar_page,
      .gl_canvas = *impl_->gl_canvas,
  };

  main_window_binder::Bind(impl_->event_bus, components);
  main_window_binder::SendInitialValues(components);
}

inline void MainWindow::ConfigureAutoExitTimer() {
  wxString exit_after_ms_env;
  if (!wxGetEnv("DECADE_EXIT_AFTER_MS", &exit_after_ms_env)) {
    return;
  }

  const std::string exit_after_ms_str = exit_after_ms_env.ToStdString();
  try {
    const std::int64_t exit_after_ms = std::stoll(exit_after_ms_str);
    if (exit_after_ms > 0) {
      std::cout << "Auto-exit in ms: " << exit_after_ms << '\n';
      exit_timer.Bind(wxEVT_TIMER, &MainWindow::OnExitTimer, this);
      exit_timer.StartOnce(static_cast<int>(exit_after_ms));
    }
  } catch (const std::exception& ex) {
    std::cout << "Invalid DECADE_EXIT_AFTER_MS: " << ex.what() << '\n';
  }
}

inline void MainWindow::InitMenu() {
  const int id_export_png = wxWindow::NewControlId();
  const int id_import_csv = wxWindow::NewControlId();
  const int id_export_csv = wxWindow::NewControlId();
  const int id_open_xml = wxWindow::NewControlId();
  const int id_license_info = wxWindow::NewControlId();

  Bind(wxEVT_MENU, &MainWindow::CallbackLoadXML, this, id_open_xml);
  Bind(wxEVT_MENU, &MainWindow::CallbackSaveXML, this, id_save_xml);
  Bind(wxEVT_MENU, &MainWindow::CallbackSaveXML, this, id_save_as_xml);
  Bind(wxEVT_MENU, &MainWindow::CallbackImportCSV, this, id_import_csv);
  Bind(wxEVT_MENU, &MainWindow::CallbackExportCSV, this, id_export_csv);
  Bind(wxEVT_MENU, &MainWindow::CallbackExportPNG, this, id_export_png);
  Bind(wxEVT_MENU, &MainWindow::CallbackExit, this, wxID_EXIT);
  Bind(wxEVT_MENU, &MainWindow::CallbackLicenseInfo, this, id_license_info);

  auto menu_bar = std::make_unique<wxMenuBar>();
  auto* menu_bar_ptr = menu_bar.release();
  SetMenuBar(menu_bar_ptr);

  auto menu_file = std::make_unique<wxMenu>();
  auto* menu_file_ptr = menu_file.release();
  menu_bar_ptr->Append(menu_file_ptr, "&File");
  menu_file_ptr->Append(id_open_xml, L"&Open...");
  menu_file_ptr->AppendSeparator();
  menu_file_ptr->Append(id_save_xml, L"&Save \tCTRL+S");
  menu_file_ptr->Append(id_save_as_xml, L"&Save As...");
  menu_file_ptr->AppendSeparator();
  menu_file_ptr->Append(id_import_csv, L"&Import csv...");
  menu_file_ptr->Append(id_export_csv, L"&Export csv...");
  menu_file_ptr->AppendSeparator();
  menu_file_ptr->Append(id_export_png, L"&Export png (600 dpi)...");
  menu_file_ptr->AppendSeparator();
  menu_file_ptr->Append(wxID_EXIT);

  auto menu_help = std::make_unique<wxMenu>();
  auto* menu_help_ptr = menu_help.release();
  menu_bar_ptr->Append(menu_help_ptr, "&Help");
  menu_help_ptr->Append(id_license_info, L"&Open Source Licenses");
}

inline void MainWindow::CallbackLoadXML(wxCommandEvent& event) {
  (void)event;
  // NOLINTNEXTLINE(misc-include-cleaner)
  wxFileDialog open_file_dialog(this, "Open File", wxEmptyString, wxEmptyString,
                                "XML Files (*.xml)|*.xml",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (open_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = open_file_dialog.GetPath().ToStdString();
  LoadXML(file_path);
  xml_file_path = file_path;
}

inline void MainWindow::CallbackSaveXML(wxCommandEvent& event) {
  if (event.GetId() == id_save_xml && !xml_file_path.empty()) {
    SaveXML(xml_file_path);
    return;
  }

  if (event.GetId() != id_save_as_xml && !xml_file_path.empty()) {
    return;
  }

  wxFileDialog save_file_dialog(this, "Save File", wxEmptyString, wxEmptyString,
                                "XML Files (*.xml)|*.xml",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (save_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = save_file_dialog.GetPath().ToStdString();
  SaveXML(file_path);
  xml_file_path = file_path;
}

inline void MainWindow::LoadXML(const std::string& filepath) {
  app::io::LoadProjectXml(filepath, impl_->date_groups_store,
                          impl_->date_interval_bundle_store,
                          impl_->page_setup_store, impl_->title_config_store,
                          impl_->shape_configuration_storage,
                          impl_->calendar_configuration_storage);
}

inline void MainWindow::SaveXML(const std::string& filepath) {
  app::io::SaveProjectXml(filepath, impl_->date_groups_store,
                          impl_->date_interval_bundle_store,
                          impl_->page_setup_store, impl_->title_config_store,
                          impl_->shape_configuration_storage,
                          impl_->calendar_configuration_storage);
}

inline void MainWindow::CallbackImportCSV(wxCommandEvent& event) {
  (void)event;
  wxFileDialog open_file_dialog(this, "Import file", wxEmptyString,
                                wxEmptyString,
                                "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (open_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = open_file_dialog.GetPath().ToStdString();
  impl_->date_interval_bundle_store.ReceiveDateIntervalBundles(
      app::io::ReadDateIntervalBundlesFromCsv(file_path));
}

inline void MainWindow::CallbackExportCSV(wxCommandEvent& event) {
  (void)event;
  wxFileDialog save_file_dialog(this, "Export file", wxEmptyString,
                                wxEmptyString,
                                "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (save_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = save_file_dialog.GetPath().ToStdString();
  app::io::WriteDateIntervalBundlesToCsv(
      file_path, impl_->date_interval_bundle_store.GetDateIntervalBundles());
}

inline void MainWindow::CallbackExportPNG(wxCommandEvent& event) {
  (void)event;
  wxFileDialog file_dialog(this, "Export PNG file", wxEmptyString,
                           wxEmptyString, "PNG files (*.png)|*.png",
                           wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = file_dialog.GetPath().ToStdString();
  impl_->gl_canvas->SavePNG(file_path);
}

inline void MainWindow::CallbackExit(wxCommandEvent& event) {
  (void)event;
  Close(true);
}

inline void MainWindow::OnExitTimer(wxTimerEvent& event) {
  (void)event;
  Close(true);
}

inline void MainWindow::CallbackLicenseInfo(wxCommandEvent& event) {
  (void)event;
  LicenseInformationDialog dialog(this);
  dialog.ShowModal();
}

#endif  // MAIN_WINDOW_HPP
