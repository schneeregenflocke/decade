#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <wx/bitmap.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/weakref.h>
#include <wx/window.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "../gui/calendar_panel.hpp"
#include "../gui/date_panel.hpp"
#include "../gui/document_panel.hpp"
#include "../gui/font_panel.hpp"
#include "../gui/groups_panel.hpp"
#include "../gui/license_panel.hpp"
#include "../gui/main_menu.hpp"
#include "../gui/opengl_panel.hpp"
#include "../gui/page_panel.hpp"
#include "../gui/scene_tree_panel.hpp"
#include "../gui/title_panel.hpp"
#include "../gui/wx_owned.hpp"
#include "../packages/calendar_config_store.hpp"
#include "../packages/date_entry_store.hpp"
#include "../packages/date_format.hpp"
#include "../packages/date_group_store.hpp"
#include "../packages/page_setup_store.hpp"
#include "../packages/shape_configuration_store.hpp"
#include "../packages/title_config_store.hpp"
#include "../packages/transform_date_entry.hpp"
#include "binding/calendar_page.hpp"
#include "binding/event_bus.hpp"
#include "binding/interaction_controller.hpp"
#include "binding/main_window_binder.hpp"
#include "runtime_options.hpp"
#include "services/csv_io.hpp"
#include "services/project_io.hpp"
#include "services/runtime_info.hpp"

class MainWindow : public wxFrame {
 public:
  MainWindow(wxWindow* parent, const wxString& title, const wxPoint& pos,
             const wxSize& size, bool maximize_on_start = true,
             app::RuntimeOptions runtime_options = {});
  ~MainWindow() override;
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;
  MainWindow(MainWindow&&) = delete;
  MainWindow& operator=(MainWindow&&) = delete;

 private:
  void CreateLayout(bool maximize_on_start);
  void CreatePanels(wxNotebook* notebook);
  void SelectStartupTab();
  void InitializeOpenGL();
  void EstablishConnections();
  void LoadStartupFile();
  void ConfigureAutoExitTimer();
  void DumpPngIfRequested();
  void DumpFramePng(const std::string& path);
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

  std::string xml_file_path_;
  wxTimer exit_timer_;

  MainMenu menu_;
  app::RuntimeOptions runtime_options_;
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
  wxWeakRef<PageSetupPanel> page_setup_panel;
  wxWeakRef<TitleSetupPanel> title_setup_panel;
  wxWeakRef<CalendarSetupPanel> calendar_setup_panel;
  wxWeakRef<GLCanvas> gl_canvas;
  wxWeakRef<FontPanel> font_panel;
  wxWeakRef<DateTablePanel> data_table_panel;
  wxWeakRef<SceneTreePanel> scene_tree_panel;

  // Application-wide locale date formatter: constructed once here and handed
  // by reference to every consumer (date table panel, CSV import/export) so
  // the locale configuration lives in exactly one place.
  LocaleDateFormatter locale_date_format;

  DateGroupStore date_groups_store;
  DateEntryStore date_entry_store;
  TransformDateEntry transform_date_entry;
  PageSetupStore page_setup_store;
  TitleConfigStore title_config_store;
  ShapeConfigurationStore shape_configuration_store;
  CalendarConfigStore calendar_configuration_store;
  std::unique_ptr<CalendarPage> calendar_page;
  InteractionController interaction_controller;
};

inline MainWindow::MainWindow(wxWindow* parent, const wxString& title,
                              const wxPoint& pos, const wxSize& size,
                              bool maximize_on_start,
                              app::RuntimeOptions runtime_options)
    : wxFrame(parent, wxID_ANY, title, pos, size),
      impl_(std::make_unique<Impl>()),
      exit_timer_(this),
      menu_(GLCanvas::kExportPngDpi),
      runtime_options_(std::move(runtime_options)) {
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
  SelectStartupTab();
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
      MakeOwned<DateTablePanel>(notebook, impl_->locale_date_format);
  impl_->date_groups_table_panel = MakeOwned<DateGroupsTablePanel>(notebook);
  impl_->calendar_setup_panel = MakeOwned<CalendarSetupPanel>(notebook);
  impl_->scene_tree_panel = MakeOwned<SceneTreePanel>(notebook);

  // Page, font and title settings are merged into a single "Document" tab; the
  // composite panel owns the three child panels, which the binder still wires
  // individually via the weak references below.
  auto* document_setup_panel = MakeOwned<DocumentSetupPanel>(notebook);
  impl_->page_setup_panel = document_setup_panel->GetPageSetupPanel();
  impl_->font_panel = document_setup_panel->GetFontPanel();
  impl_->title_setup_panel = document_setup_panel->GetTitleSetupPanel();

  notebook->AddPage(impl_->date_groups_table_panel, "Categories");
  notebook->AddPage(impl_->data_table_panel, "Entries");
  notebook->AddPage(document_setup_panel, "Document");
  notebook->AddPage(impl_->calendar_setup_panel, "Timeframe");
  notebook->AddPage(impl_->scene_tree_panel, "Scene");
}

inline void MainWindow::SelectStartupTab() {
  if (!runtime_options_.select_tab || impl_->notebook == nullptr) {
    return;
  }
  const wxString wanted = wxString::FromUTF8(*runtime_options_.select_tab);
  for (size_t index = 0; index < impl_->notebook->GetPageCount(); ++index) {
    if (impl_->notebook->GetPageText(index).IsSameAs(wanted, false)) {
      impl_->notebook->SetSelection(index);
      return;
    }
  }
  std::cerr << "DECADE_SELECT_TAB: no tab labelled '"
            << *runtime_options_.select_tab << "'\n";
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
    LoadStartupFile();
    if (runtime_options_.debug_hover_bar) {
      impl_->calendar_page->ReceiveHovered(
          PickId{.kind = PickId::Kind::kBar,
                 .index = *runtime_options_.debug_hover_bar});
    }
    if (runtime_options_.debug_select_node) {
      // Drive the real selection path (tree -> detail grid -> bus -> highlight)
      // so this exercises the panel exactly as a click would.
      impl_->scene_tree_panel->SelectNodeByPath(
          *runtime_options_.debug_select_node);
    }
    DumpPngIfRequested();
  });
}

inline void MainWindow::LoadStartupFile() {
  // Opt-in: a file is loaded only when one was supplied, either as the
  // positional command-line argument or via DECADE_DEFAULT_CSV. With neither
  // set the application starts with an empty project. The caller is responsible
  // for the path, so there is no hidden working-directory-relative default.
  if (!runtime_options_.startup_file) {
    return;
  }
  const std::string& path = *runtime_options_.startup_file;

  if (!wxFileExists(path)) {
    std::cout << "LoadStartupFile: " << path << " not found, skipping\n";
    return;
  }

  std::cout << "LoadStartupFile: loading " << path << '\n';
  if (path.ends_with(".xml")) {
    LoadXML(path);
    xml_file_path_ = path;
  } else {
    impl_->date_entry_store.ReceiveDateEntries(
        app::io::ReadDateEntriesFromCsv(path, impl_->locale_date_format));
  }
}

inline void MainWindow::DumpPngIfRequested() {
  if (runtime_options_.dump_png_path) {
    const std::string& path = *runtime_options_.dump_png_path;
    const int dpi =
        runtime_options_.dump_png_dpi.value_or(GLCanvas::kExportPngDpi);
    std::cout << "DECADE_DUMP_PNG: writing " << path << " at " << dpi
              << " dpi\n";
    impl_->gl_canvas->SavePNG(path, dpi);
  }
  if (runtime_options_.dump_window_png_path) {
    const std::string path = *runtime_options_.dump_window_png_path;
    // Defer until after the first real paint so the back buffer is populated.
    CallAfter([this, path]() {
      std::cout << "DECADE_DUMP_WINDOW_PNG: writing " << path << '\n';
      impl_->gl_canvas->SaveWindowPNG(path);
    });
  }
  if (runtime_options_.dump_frame_png_path) {
    const std::string path = *runtime_options_.dump_frame_png_path;
    // Defer until after the first real paint so every panel has drawn itself.
    CallAfter([this, path]() {
      std::cout << "DECADE_DUMP_FRAME_PNG: writing " << path << '\n';
      DumpFramePng(path);
    });
  }
}

inline void MainWindow::DumpFramePng(const std::string& path) {
  const wxSize size = GetClientSize();
  if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
    return;
  }

  // Grab the wx widget tree (menu bar, tabs, panels) through a client DC.
  wxBitmap bitmap(size.GetWidth(), size.GetHeight());
  {
    wxClientDC client_dc(this);
    wxMemoryDC memory_dc(bitmap);
    memory_dc.Blit(0, 0, size.GetWidth(), size.GetHeight(), &client_dc, 0, 0);
  }

  // The OpenGL canvas is invisible to a wxDC, so paste its back buffer on top
  // at the canvas's position within the frame.
  if (impl_->gl_canvas.get() != nullptr) {
    const wxImage gl_image = impl_->gl_canvas->CaptureBackBufferImage();
    if (gl_image.IsOk()) {
      const wxPoint origin =
          ScreenToClient(impl_->gl_canvas->GetScreenPosition());
      const wxSize canvas_size = impl_->gl_canvas->GetSize();
      const wxImage fitted =
          (gl_image.GetWidth() == canvas_size.GetWidth() &&
           gl_image.GetHeight() == canvas_size.GetHeight())
              ? gl_image
              : gl_image.Scale(canvas_size.GetWidth(), canvas_size.GetHeight());
      wxMemoryDC memory_dc(bitmap);
      memory_dc.DrawBitmap(wxBitmap(fitted), origin, false);
    }
  }

  if (wxImage::FindHandler(wxBITMAP_TYPE_PNG) == nullptr) {
    wxImage::AddHandler(MakeOwned<wxPNGHandler>());
  }
  if (!bitmap.ConvertToImage().SaveFile(path, wxBITMAP_TYPE_PNG)) {
    std::cerr << "DECADE_DUMP_FRAME_PNG: failed to write " << path << '\n';
  }
}

inline void MainWindow::EstablishConnections() {
  MainWindowComponents components{
      .date_groups_store = impl_->date_groups_store,
      .date_entry_store = impl_->date_entry_store,
      .transform_date_entry = impl_->transform_date_entry,
      .page_setup_store = impl_->page_setup_store,
      .title_config_store = impl_->title_config_store,
      .shape_configuration_store = impl_->shape_configuration_store,
      .calendar_configuration_store = impl_->calendar_configuration_store,
      .data_table_panel = *impl_->data_table_panel,
      .date_groups_table_panel = *impl_->date_groups_table_panel,
      .page_setup_panel = *impl_->page_setup_panel,
      .title_setup_panel = *impl_->title_setup_panel,
      .calendar_setup_panel = *impl_->calendar_setup_panel,
      .font_panel = *impl_->font_panel,
      .scene_tree_panel = *impl_->scene_tree_panel,
      .calendar_page = *impl_->calendar_page,
      .gl_canvas = *impl_->gl_canvas,
      .interaction_controller = impl_->interaction_controller,
  };

  main_window_binder::Bind(impl_->event_bus, components);
  main_window_binder::SendInitialValues(components);
}

inline void MainWindow::ConfigureAutoExitTimer() {
  if (!runtime_options_.exit_after_ms) {
    return;
  }

  const std::int64_t exit_after_ms = *runtime_options_.exit_after_ms;
  std::cout << "Auto-exit in ms: " << exit_after_ms << '\n';
  exit_timer_.Bind(wxEVT_TIMER, &MainWindow::OnExitTimer, this);
  exit_timer_.StartOnce(static_cast<int>(exit_after_ms));
}

inline void MainWindow::InitMenu() {
  const MainMenuIds& ids = menu_.Ids();

  Bind(wxEVT_MENU, &MainWindow::CallbackLoadXML, this, ids.open_xml);
  Bind(wxEVT_MENU, &MainWindow::CallbackSaveXML, this, ids.save_xml);
  Bind(wxEVT_MENU, &MainWindow::CallbackSaveXML, this, ids.save_as_xml);
  Bind(wxEVT_MENU, &MainWindow::CallbackImportCSV, this, ids.import_csv);
  Bind(wxEVT_MENU, &MainWindow::CallbackExportCSV, this, ids.export_csv);
  Bind(wxEVT_MENU, &MainWindow::CallbackExportPNG, this, ids.export_png);
  Bind(wxEVT_MENU, &MainWindow::CallbackExit, this, wxID_EXIT);
  Bind(wxEVT_MENU, &MainWindow::CallbackLicenseInfo, this, ids.license_info);

  menu_.AttachTo(*this);
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
  xml_file_path_ = file_path;
}

inline void MainWindow::CallbackSaveXML(wxCommandEvent& event) {
  const MainMenuIds& ids = menu_.Ids();
  if (event.GetId() == ids.save_xml && !xml_file_path_.empty()) {
    SaveXML(xml_file_path_);
    return;
  }

  if (event.GetId() != ids.save_as_xml && !xml_file_path_.empty()) {
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
  xml_file_path_ = file_path;
}

inline void MainWindow::LoadXML(const std::string& filepath) {
  app::io::LoadProjectXml(
      filepath, impl_->date_groups_store, impl_->date_entry_store,
      impl_->page_setup_store, impl_->title_config_store,
      impl_->shape_configuration_store, impl_->calendar_configuration_store);
}

inline void MainWindow::SaveXML(const std::string& filepath) {
  app::io::SaveProjectXml(
      filepath, impl_->date_groups_store, impl_->date_entry_store,
      impl_->page_setup_store, impl_->title_config_store,
      impl_->shape_configuration_store, impl_->calendar_configuration_store);
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
  impl_->date_entry_store.ReceiveDateEntries(
      app::io::ReadDateEntriesFromCsv(file_path, impl_->locale_date_format));
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
  app::io::WriteDateEntriesToCsv(file_path,
                                 impl_->date_entry_store.GetDateEntries(),
                                 impl_->locale_date_format);
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
