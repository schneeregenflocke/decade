#include "main_window.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <wx/defs.h>
#include <wx/event.h>
#include <wx/filedlg.h>
#include <wx/gdicmn.h>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/string.h>
#include <wx/txtstrm.h>
#include <wx/utils.h>
#include <wx/weakref.h>
#include <wx/window.h>

#include "../calendar_page.hpp"
#include "../gui/calendar_panel.hpp"
#include "../gui/date_panel.hpp"
#include "../gui/font_panel.hpp"
#include "../gui/groups_panel.hpp"
#include "../gui/license_panel.hpp"
#include "../gui/log_panel.hpp"
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
#include "services/project_io.hpp"

namespace {
constexpr int kOpenGLMajor = 4;
constexpr int kOpenGLMinor = 6;
} // namespace

struct MainWindow::Impl {
  wxWeakRef<wxSplitterWindow> main_splitter;
  wxWeakRef<wxNotebook> notebook;
  std::unique_ptr<DateGroupsTablePanel> date_groups_table_panel;
  std::unique_ptr<ElementsSetupsPanel> elements_setup_panel;
  std::unique_ptr<PageSetupPanel> page_setup_panel;
  std::unique_ptr<TitleSetupPanel> title_setup_panel;
  std::unique_ptr<CalendarSetupPanel> calendar_setup_panel;
  std::unique_ptr<GLCanvas> gl_canvas;
  std::unique_ptr<FontPanel> font_panel;
  std::unique_ptr<LogPanel> log_panel;
  std::unique_ptr<DateTablePanel> data_table_panel;

  DateGroupStore date_groups_store;
  DateIntervalBundleStore date_interval_bundle_store;
  TransformDateIntervalBundle transform_date_interval_bundle;
  PageSetupStore page_setup_store;
  TitleConfigStore title_config_store;
  ShapeConfigurationStorage shape_configuration_storage;
  CalendarConfigStorage calendar_configuration_storage;
  std::unique_ptr<CalendarPage> calendar_page;
};

MainWindow::MainWindow(wxWindow *parent, const wxString &title, const wxPoint &pos,
                       const wxSize &size, bool maximize_on_start)
    : wxFrame(parent, wxID_ANY, title, pos, size), impl_(std::make_unique<Impl>()),
      exit_timer(this), id_save_xml(wxWindow::NewControlId()),
      id_save_as_xml(wxWindow::NewControlId())
{
  CreateLayout(maximize_on_start);
  InitMenu();
  InitializeOpenGL();
  ConfigureAutoExitTimer();
}

MainWindow::~MainWindow() = default;

void MainWindow::CreateLayout(bool maximize_on_start)
{
  if (maximize_on_start) {
    Maximize();
  }

  auto main_splitter = std::make_unique<wxSplitterWindow>(
      this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
  impl_->main_splitter = main_splitter.get();
  auto *main_splitter_ptr = main_splitter.release();

  constexpr int minimum_pane_size = 5;
  constexpr double sash_gravity = 0.5;
  main_splitter_ptr->SetMinimumPaneSize(minimum_pane_size);
  main_splitter_ptr->SetSashGravity(sash_gravity);

  constexpr int kSizerBorder = 5;
  wxSizerFlags sizer_flags;
  sizer_flags.Proportion(1).Expand().Border(wxALL, kSizerBorder);

  auto notebook_panel = std::make_unique<wxPanel>(main_splitter_ptr, wxID_ANY);
  auto *notebook_panel_ptr = notebook_panel.release();
  auto notebook_panel_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
  auto *notebook_panel_sizer_ptr = notebook_panel_sizer.release();
  notebook_panel_ptr->SetSizer(notebook_panel_sizer_ptr);

  auto notebook = std::make_unique<wxNotebook>(notebook_panel_ptr, wxID_ANY, wxDefaultPosition,
                                               wxDefaultSize, wxNB_TOP);
  impl_->notebook = notebook.get();
  auto *notebook_ptr = notebook.release();
  notebook_panel_sizer_ptr->Add(notebook_ptr, sizer_flags);

  CreatePanels(notebook_ptr);
  app::io::PrintRuntimeInfo(std::cout);

  auto gl_canvas_panel = std::make_unique<wxPanel>(main_splitter_ptr, wxID_ANY);
  auto *gl_canvas_panel_ptr = gl_canvas_panel.release();
  impl_->gl_canvas = std::make_unique<GLCanvas>(gl_canvas_panel_ptr);
  auto gl_canvas_panel_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
  auto *gl_canvas_panel_sizer_ptr = gl_canvas_panel_sizer.release();
  gl_canvas_panel_ptr->SetSizer(gl_canvas_panel_sizer_ptr);
  gl_canvas_panel_sizer_ptr->Add(impl_->gl_canvas->GLCanvasPtr(), sizer_flags);

  main_splitter_ptr->SplitVertically(notebook_panel_ptr, gl_canvas_panel_ptr);
}

void MainWindow::CreatePanels(wxNotebook *notebook)
{
  impl_->log_panel = std::make_unique<LogPanel>(notebook);
  notebook->AddPage(impl_->log_panel->PanelPtr(), "Log");

  impl_->data_table_panel = std::make_unique<DateTablePanel>(notebook);
  impl_->date_groups_table_panel = std::make_unique<DateGroupsTablePanel>(notebook);
  impl_->calendar_setup_panel = std::make_unique<CalendarSetupPanel>(notebook);
  impl_->elements_setup_panel = std::make_unique<ElementsSetupsPanel>(notebook);
  impl_->page_setup_panel = std::make_unique<PageSetupPanel>(notebook);
  impl_->font_panel = std::make_unique<FontPanel>(notebook);
  impl_->title_setup_panel = std::make_unique<TitleSetupPanel>(notebook);

  notebook->AddPage(impl_->data_table_panel->PanelPtr(), "Dates");
  notebook->AddPage(impl_->date_groups_table_panel->PanelPtr(), "Groups");
  notebook->AddPage(impl_->calendar_setup_panel->PanelPtr(), "Calendar");
  notebook->AddPage(impl_->elements_setup_panel->PanelPtr(), "Shapes");
  notebook->AddPage(impl_->page_setup_panel->PanelPtr(), "Page");
  notebook->AddPage(impl_->font_panel->PanelPtr(), "Font");
  notebook->AddPage(impl_->title_setup_panel->PanelPtr(), "Title");
}

void MainWindow::InitializeOpenGL()
{
  CreateStatusBar(1);
  Show();
  Raise();

  const std::array<int, 2> gl_version{kOpenGLMajor, kOpenGLMinor};
  impl_->gl_canvas->InitOpenGL(gl_version, [this]() {
    impl_->calendar_page = std::make_unique<CalendarPage>(impl_->gl_canvas.get(),
                                                          impl_->font_panel->GetFontFilePath());
    EstablishConnections();
  });
}

void MainWindow::EstablishConnections()
{
  impl_->date_interval_bundle_store.SignalDateIntervalBundles().connect(
      &DateTablePanel::ReceiveDateIntervalBundles, impl_->data_table_panel.get());
  impl_->data_table_panel->SignalTableDateIntervalBundles().connect(
      &DateIntervalBundleStore::ReceiveDateIntervalBundles, &impl_->date_interval_bundle_store);

  impl_->date_interval_bundle_store.SignalDateIntervalBundles().connect(
      &TransformDateIntervalBundle::ReceiveDateIntervalBundles,
      &impl_->transform_date_interval_bundle);
  impl_->transform_date_interval_bundle.SetTransform({0, 1});
  impl_->transform_date_interval_bundle.SignalTransformDateIntervalBundles().connect(
      &CalendarPage::ReceiveDateIntervalBundles, impl_->calendar_page.get());

  impl_->date_groups_store.SignalDateGroups().connect(&DateGroupsTablePanel::ReceiveDateGroups,
                                                      impl_->date_groups_table_panel.get());
  impl_->date_groups_table_panel->SignalTableDateGroups().connect(
      &DateGroupStore::ReceiveDateGroups, &impl_->date_groups_store);

  impl_->date_groups_store.SignalDateGroups().connect(&DateIntervalBundleStore::ReceiveDateGroups,
                                                      &impl_->date_interval_bundle_store);
  impl_->date_groups_store.SignalDateGroups().connect(&DateTablePanel::ReceiveDateGroups,
                                                      impl_->data_table_panel.get());
  impl_->date_groups_store.SignalDateGroups().connect(&ElementsSetupsPanel::ReceiveDateGroups,
                                                      impl_->elements_setup_panel.get());
  impl_->date_groups_store.SignalDateGroups().connect(&CalendarPage::ReceiveDateGroups,
                                                      impl_->calendar_page.get());

  impl_->page_setup_store.signal_page_setup_config.connect(&PageSetupPanel::ReceivePageSetup,
                                                           impl_->page_setup_panel.get());
  impl_->page_setup_panel->signal_page_setup_config.connect(&PageSetupStore::ReceivePageSetup,
                                                            &impl_->page_setup_store);

  impl_->page_setup_store.signal_page_setup_config.connect(&CalendarPage::ReceivePageSetup,
                                                           impl_->calendar_page.get());
  impl_->page_setup_store.signal_page_setup_config.connect(&GLCanvas::ReceivePageSetup,
                                                           impl_->gl_canvas.get());

  impl_->font_panel->signal_font_filepath.connect(&CalendarPage::ReceiveFont,
                                                  impl_->calendar_page.get());

  impl_->title_config_store.SignalTitleConfig().connect(&TitleSetupPanel::ReceiveTitleConfig,
                                                        impl_->title_setup_panel.get());
  impl_->title_setup_panel->SignalTitleConfig().connect(&TitleConfigStore::ReceiveTitleConfig,
                                                        &impl_->title_config_store);

  impl_->title_config_store.SignalTitleConfig().connect(&CalendarPage::ReceiveTitleConfig,
                                                        impl_->calendar_page.get());

  impl_->shape_configuration_storage.SignalShapeConfigurationStorage().connect(
      &ElementsSetupsPanel::ReceiveShapeConfigurationStorage, impl_->elements_setup_panel.get());
  impl_->elements_setup_panel->SignalShapeConfigurationStorage().connect(
      &ShapeConfigurationStorage::ReceiveShapeConfigurationStorage,
      &impl_->shape_configuration_storage);

  impl_->shape_configuration_storage.SignalShapeConfigurationStorage().connect(
      &CalendarPage::ReceiveShapeConfigurationStorage, impl_->calendar_page.get());

  impl_->calendar_configuration_storage.SignalCalendarConfigStorage().connect(
      &CalendarSetupPanel::ReceiveCalendarConfigStorage, impl_->calendar_setup_panel.get());
  impl_->calendar_setup_panel->SignalCalendarConfigStorage().connect(
      &CalendarConfigStorage::ReceiveCalendarConfigStorage, &impl_->calendar_configuration_storage);

  impl_->calendar_configuration_storage.SignalCalendarConfigStorage().connect(
      &CalendarPage::ReceiveCalendarConfig, impl_->calendar_page.get());

  impl_->shape_configuration_storage.SendShapeConfigurationStorage();
  impl_->date_groups_store.SendDefaultValues();
  impl_->page_setup_panel->SendDefaultValues();
  impl_->title_setup_panel->SendDefaultValues();
  impl_->calendar_configuration_storage.SendCalendarConfigStorage();
}

void MainWindow::ConfigureAutoExitTimer()
{
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
  } catch (const std::exception &ex) {
    std::cout << "Invalid DECADE_EXIT_AFTER_MS: " << ex.what() << '\n';
  }
}

void MainWindow::InitMenu()
{
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
  auto *menu_bar_ptr = menu_bar.release();
  SetMenuBar(menu_bar_ptr);

  auto menu_file = std::make_unique<wxMenu>();
  auto *menu_file_ptr = menu_file.release();
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
  auto *menu_help_ptr = menu_help.release();
  menu_bar_ptr->Append(menu_help_ptr, "&Help");
  menu_help_ptr->Append(id_license_info, L"&Open Source Licenses");
}

void MainWindow::CallbackLoadXML(wxCommandEvent &event)
{
  (void)event;
  wxFileDialog open_file_dialog(this, "Open File", wxEmptyString, wxEmptyString,
                                "XML Files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (open_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = open_file_dialog.GetPath().ToStdString();
  LoadXML(file_path);
  xml_file_path = file_path;
}

void MainWindow::CallbackSaveXML(wxCommandEvent &event)
{
  if (event.GetId() == id_save_xml && !xml_file_path.empty()) {
    SaveXML(xml_file_path);
    return;
  }

  if (event.GetId() != id_save_as_xml && !xml_file_path.empty()) {
    return;
  }

  wxFileDialog save_file_dialog(this, "Save File", wxEmptyString, wxEmptyString,
                                "XML Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (save_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = save_file_dialog.GetPath().ToStdString();
  SaveXML(file_path);
  xml_file_path = file_path;
}

void MainWindow::LoadXML(const std::string &filepath)
{
  app::io::LoadProjectXml(filepath, impl_->date_groups_store, impl_->date_interval_bundle_store,
                          impl_->page_setup_store, impl_->title_config_store,
                          impl_->shape_configuration_storage,
                          impl_->calendar_configuration_storage);
}

void MainWindow::SaveXML(const std::string &filepath)
{
  app::io::SaveProjectXml(filepath, impl_->date_groups_store, impl_->date_interval_bundle_store,
                          impl_->page_setup_store, impl_->title_config_store,
                          impl_->shape_configuration_storage,
                          impl_->calendar_configuration_storage);
}

void MainWindow::CallbackImportCSV(wxCommandEvent &event)
{
  (void)event;
  wxFileDialog open_file_dialog(this, "Import file", wxEmptyString, wxEmptyString,
                                "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (open_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = open_file_dialog.GetPath().ToStdString();
  impl_->date_interval_bundle_store.ReceiveDateIntervalBundles(
      app::io::ReadDateIntervalBundlesFromCsv(file_path));
}

void MainWindow::CallbackExportCSV(wxCommandEvent &event)
{
  (void)event;
  wxFileDialog save_file_dialog(this, "Export file", wxEmptyString, wxEmptyString,
                                "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (save_file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = save_file_dialog.GetPath().ToStdString();
  app::io::WriteDateIntervalBundlesToCsv(
      file_path, impl_->date_interval_bundle_store.GetDateIntervalBundles());
}

void MainWindow::CallbackExportPNG(wxCommandEvent &event)
{
  (void)event;
  wxFileDialog file_dialog(this, "Export PNG file", wxEmptyString, wxEmptyString,
                           "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  const std::string file_path = file_dialog.GetPath().ToStdString();
  impl_->gl_canvas->SavePNG(file_path);
}

void MainWindow::CallbackExit(wxCommandEvent &event)
{
  (void)event;
  Close(true);
}

void MainWindow::OnExitTimer(wxTimerEvent &event)
{
  (void)event;
  Close(true);
}

void MainWindow::CallbackLicenseInfo(wxCommandEvent &event)
{
  (void)event;
  LicenseInformationDialog dialog(this);
  dialog.ShowModal();
}
