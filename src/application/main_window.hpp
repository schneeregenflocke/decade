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

#include "../presentation/calendar_panel.hpp"
#include "../presentation/date_panel.hpp"
#include "../presentation/document_panel.hpp"
#include "../presentation/font_panel.hpp"
#include "../presentation/groups_panel.hpp"
#include "../presentation/license_panel.hpp"
#include "../presentation/main_menu.hpp"
#include "../presentation/gl_canvas.hpp"
#include "../presentation/page_panel.hpp"
#include "../presentation/scene_tree_panel.hpp"
#include "../presentation/title_panel.hpp"
#include "../presentation/wx_owned.hpp"
#include "../domain/calendar_config_store.hpp"
#include "../domain/date_entry_store.hpp"
#include "../domain/date_format.hpp"
#include "../domain/date_group_store.hpp"
#include "../domain/page_setup_store.hpp"
#include "../domain/shape_configuration_store.hpp"
#include "../domain/title_config_store.hpp"
#include "../domain/transform_date_entry.hpp"
#include "calendar/calendar_page.hpp"
#include "event_bus.hpp"
#include "calendar/interaction_controller.hpp"
#include "main_window_binder.hpp"
#include "runtime_options.hpp"
#include "../infrastructure/persistence/csv_io.hpp"
#include "../infrastructure/persistence/project_io.hpp"
#include "runtime_info.hpp"

class MainWindow : public wxFrame {
 public:
  MainWindow(wxWindow* parent, const wxString& title, const wxPoint& pos,
             const wxSize& size, bool maximize_on_start = true,
             application::RuntimeOptions runtime_options = {});
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

  // Declared first so it is destroyed *last* — every producer (stores,
  // panels, the GL canvas) emits via `event_bus_`, and some emissions can run
  // during teardown. Keeping the bus alive longer than its capturing lambdas
  // avoids dangling references in slot callbacks.
  EventBus event_bus_;

  wxWeakRef<wxSplitterWindow> main_splitter_;
  wxWeakRef<wxNotebook> notebook_;

  wxWeakRef<DateGroupsTablePanel> date_groups_table_panel_;
  wxWeakRef<PageSetupPanel> page_setup_panel_;
  wxWeakRef<TitleSetupPanel> title_setup_panel_;
  wxWeakRef<CalendarSetupPanel> calendar_setup_panel_;
  wxWeakRef<GLCanvas> gl_canvas_;
  wxWeakRef<FontPanel> font_panel_;
  wxWeakRef<DateTablePanel> data_table_panel_;
  wxWeakRef<SceneTreePanel> scene_tree_panel_;

  // Application-wide locale date formatter: constructed once here and handed
  // by reference to every consumer (date table panel, CSV import/export) so
  // the locale configuration lives in exactly one place.
  LocaleDateFormatter locale_date_format_;

  DateGroupStore date_groups_store_;
  DateEntryStore date_entry_store_;
  TransformDateEntry transform_date_entry_;
  PageSetupStore page_setup_store_;
  TitleConfigStore title_config_store_;
  ShapeConfigurationStore shape_configuration_store_;
  CalendarConfigStore calendar_configuration_store_;
  std::unique_ptr<CalendarPage> calendar_page_;
  InteractionController interaction_controller_;

  std::string xml_file_path_;
  wxTimer exit_timer_;

  MainMenu menu_;
  application::RuntimeOptions runtime_options_;
};

namespace main_window_detail {
constexpr int kOpenGLMajor = 4;
constexpr int kOpenGLMinor = 6;
}  // namespace main_window_detail

inline MainWindow::MainWindow(wxWindow* parent, const wxString& title,
                              const wxPoint& pos, const wxSize& size,
                              bool maximize_on_start,
                              application::RuntimeOptions runtime_options)
    : wxFrame(parent, wxID_ANY, title, pos, size),
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
  main_splitter_ = main_splitter.get();
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
  notebook_ = notebook.get();
  auto* notebook_ptr = notebook.release();
  notebook_panel_sizer_ptr->Add(notebook_ptr, sizer_flags);

  CreatePanels(notebook_ptr);
  SelectStartupTab();
  application::PrintRuntimeInfo(std::cout);

  auto gl_canvas_panel = std::make_unique<wxPanel>(main_splitter_ptr, wxID_ANY);
  auto* gl_canvas_panel_ptr = gl_canvas_panel.release();
  auto gl_canvas = std::make_unique<GLCanvas>(gl_canvas_panel_ptr);
  gl_canvas_ = gl_canvas.get();
  auto* gl_canvas_ptr = gl_canvas.release();
  auto gl_canvas_panel_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
  auto* gl_canvas_panel_sizer_ptr = gl_canvas_panel_sizer.release();
  gl_canvas_panel_ptr->SetSizer(gl_canvas_panel_sizer_ptr);
  gl_canvas_panel_sizer_ptr->Add(gl_canvas_ptr, sizer_flags);

  main_splitter_ptr->SplitVertically(notebook_panel_ptr, gl_canvas_panel_ptr);
}

inline void MainWindow::CreatePanels(wxNotebook* notebook) {
  data_table_panel_ = MakeOwned<DateTablePanel>(notebook, locale_date_format_);
  date_groups_table_panel_ = MakeOwned<DateGroupsTablePanel>(notebook);
  calendar_setup_panel_ = MakeOwned<CalendarSetupPanel>(notebook);
  scene_tree_panel_ = MakeOwned<SceneTreePanel>(notebook);

  // Page, font and title settings are merged into a single "Document" tab; the
  // composite panel owns the three child panels, which the binder still wires
  // individually via the weak references below.
  auto* document_setup_panel = MakeOwned<DocumentSetupPanel>(notebook);
  page_setup_panel_ = document_setup_panel->GetPageSetupPanel();
  font_panel_ = document_setup_panel->GetFontPanel();
  title_setup_panel_ = document_setup_panel->GetTitleSetupPanel();

  notebook->AddPage(date_groups_table_panel_, "Categories");
  notebook->AddPage(data_table_panel_, "Entries");
  notebook->AddPage(document_setup_panel, "Document");
  notebook->AddPage(calendar_setup_panel_, "Timeframe");
  notebook->AddPage(scene_tree_panel_, "Scene");
}

inline void MainWindow::SelectStartupTab() {
  if (!runtime_options_.select_tab || notebook_ == nullptr) {
    return;
  }
  const wxString wanted = wxString::FromUTF8(*runtime_options_.select_tab);
  for (size_t index = 0; index < notebook_->GetPageCount(); ++index) {
    if (notebook_->GetPageText(index).IsSameAs(wanted, false)) {
      notebook_->SetSelection(index);
      return;
    }
  }
  std::cerr << "--select-tab: no tab labelled '" << *runtime_options_.select_tab
            << "'\n";
}

inline void MainWindow::InitializeOpenGL() {
  CreateStatusBar(1);
  Show();
  Raise();

  const std::array<int, 2> gl_version{main_window_detail::kOpenGLMajor,
                                      main_window_detail::kOpenGLMinor};
  gl_canvas_->InitOpenGL(gl_version, [this]() {
    calendar_page_ = std::make_unique<CalendarPage>(
        gl_canvas_.get(), font_panel_->GetFontFilePath());
    EstablishConnections();
    LoadStartupFile();
    if (runtime_options_.debug_hover_bar) {
      calendar_page_->ReceiveHovered(
          PickId{.kind = PickId::Kind::kBar,
                 .index = *runtime_options_.debug_hover_bar});
    }
    if (runtime_options_.debug_select_node) {
      // Drive the real selection path (tree -> detail grid -> bus -> highlight)
      // so this exercises the panel exactly as a click would.
      scene_tree_panel_->SelectNodeByPath(*runtime_options_.debug_select_node);
    }
    DumpPngIfRequested();
  });
}

inline void MainWindow::LoadStartupFile() {
  // Opt-in: a file is loaded only when one was supplied as the positional
  // command-line argument. Without it the application starts with an empty
  // project. The caller is responsible for the path, so there is no hidden
  // working-directory-relative default.
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
    date_entry_store_.ReceiveDateEntries(
        persistence::ReadDateEntriesFromCsv(path, locale_date_format_));
  }
}

inline void MainWindow::DumpPngIfRequested() {
  if (runtime_options_.dump_png_path) {
    const std::string& path = *runtime_options_.dump_png_path;
    const int dpi =
        runtime_options_.dump_png_dpi.value_or(GLCanvas::kExportPngDpi);
    std::cout << "--dump-png: writing " << path << " at " << dpi << " dpi\n";
    gl_canvas_->SavePNG(path, dpi);
  }
  if (runtime_options_.dump_window_png_path) {
    const std::string path = *runtime_options_.dump_window_png_path;
    // Defer until after the first real paint so the back buffer is populated.
    CallAfter([this, path]() {
      std::cout << "--dump-window-png: writing " << path << '\n';
      gl_canvas_->SaveWindowPNG(path);
    });
  }
  if (runtime_options_.dump_frame_png_path) {
    const std::string path = *runtime_options_.dump_frame_png_path;
    // Defer until after the first real paint so every panel has drawn itself.
    CallAfter([this, path]() {
      std::cout << "--dump-frame-png: writing " << path << '\n';
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
  if (gl_canvas_.get() != nullptr) {
    const wxImage gl_image = gl_canvas_->CaptureBackBufferImage();
    if (gl_image.IsOk()) {
      const wxPoint origin = ScreenToClient(gl_canvas_->GetScreenPosition());
      const wxSize canvas_size = gl_canvas_->GetSize();
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
    std::cerr << "--dump-frame-png: failed to write " << path << '\n';
  }
}

inline void MainWindow::EstablishConnections() {
  MainWindowComponents components{
      .date_groups_store = date_groups_store_,
      .date_entry_store = date_entry_store_,
      .transform_date_entry = transform_date_entry_,
      .page_setup_store = page_setup_store_,
      .title_config_store = title_config_store_,
      .shape_configuration_store = shape_configuration_store_,
      .calendar_configuration_store = calendar_configuration_store_,
      .data_table_panel = *data_table_panel_,
      .date_groups_table_panel = *date_groups_table_panel_,
      .page_setup_panel = *page_setup_panel_,
      .title_setup_panel = *title_setup_panel_,
      .calendar_setup_panel = *calendar_setup_panel_,
      .font_panel = *font_panel_,
      .scene_tree_panel = *scene_tree_panel_,
      .calendar_page = *calendar_page_,
      .gl_canvas = *gl_canvas_,
      .interaction_controller = interaction_controller_,
  };

  main_window_binder::Bind(event_bus_, components);
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
  persistence::LoadProjectXml(filepath, date_groups_store_, date_entry_store_,
                          page_setup_store_, title_config_store_,
                          shape_configuration_store_,
                          calendar_configuration_store_);
}

inline void MainWindow::SaveXML(const std::string& filepath) {
  persistence::SaveProjectXml(filepath, date_groups_store_, date_entry_store_,
                          page_setup_store_, title_config_store_,
                          shape_configuration_store_,
                          calendar_configuration_store_);
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
  date_entry_store_.ReceiveDateEntries(
      persistence::ReadDateEntriesFromCsv(file_path, locale_date_format_));
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
  persistence::WriteDateEntriesToCsv(file_path, date_entry_store_.GetDateEntries(),
                                 locale_date_format_);
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
  gl_canvas_->SavePNG(file_path);
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
