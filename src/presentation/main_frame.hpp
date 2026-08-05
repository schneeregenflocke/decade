#ifndef MAIN_FRAME_HPP
#define MAIN_FRAME_HPP

#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/weakref.h>
#include <wx/window.h>

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
#include "page_panel.hpp"
#include "scene_tree_panel.hpp"
#include "title_panel.hpp"
#include "window_screenshot.hpp"
#include "wx_owned.hpp"

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
class MainFrame : public wxFrame {
 public:
  MainFrame(wxWindow* parent, const application::MainFrameConfig& config,
            LocaleDateFormatter& locale_date_formatter)
      : wxFrame(parent, wxID_ANY, wxString::FromUTF8(config.title),
                config.position, config.size, config.style, config.frame_name),
        locale_date_formatter_(locale_date_formatter),
        exit_timer_(this),
        menu_(GLCanvas::kExportPngDpi) {
    CreateLayout(config.maximize_on_start);
    InitMenu();
  }

  ~MainFrame() override = default;
  MainFrame(const MainFrame&) = delete;
  MainFrame& operator=(const MainFrame&) = delete;
  MainFrame(MainFrame&&) = delete;
  MainFrame& operator=(MainFrame&&) = delete;

  [[nodiscard]] auto& SignalFileCommand() { return signal_file_command_; }

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
  [[nodiscard]] SceneTreePanel& SceneTree() { return *scene_tree_panel_; }
  [[nodiscard]] GLCanvas& Canvas() { return *gl_canvas_; }

  // Preselects the tab with this caption (case-insensitively). It reports
  // whether the tab exists.
  [[nodiscard]] bool SelectTab(const std::string& label) {
    const wxString wanted = wxString::FromUTF8(label);
    for (size_t index = 0; index < notebook_->GetPageCount(); ++index) {
      if (notebook_->GetPageText(index).IsSameAs(wanted, false)) {
        notebook_->SetSelection(index);
        return true;
      }
    }
    return false;
  }

  // Closes the window after N milliseconds — for headless runs. The binding
  // happens at the window, not at the timer: `exit_timer_(this)` makes the
  // window the owner, and the owner gets the event. A handler on the timer
  // object would never run.
  void CloseAfter(std::int64_t milliseconds) {
    Bind(
        wxEVT_TIMER, [this](wxTimerEvent&) { Close(true); },
        exit_timer_.GetId());
    exit_timer_.StartOnce(static_cast<int>(milliseconds));
  }

  // Writes the whole window as a PNG: the widget capture plus the mounted-in GL
  // back buffer, which a wxDC does not see.
  [[nodiscard]] bool SaveFrameScreenshot(const std::string& file_path) {
    const window_screenshot::Overlay overlay{
        .image = gl_canvas_->CaptureBackBufferImage(),
        .origin = ScreenToClient(gl_canvas_->GetScreenPosition()),
        .size = gl_canvas_->GetSize()};
    return window_screenshot::SaveWindowPng(*this, overlay, file_path);
  }

 private:
  void CreateLayout(bool maximize_on_start) {
    if (maximize_on_start) {
      Maximize();
    }

    auto* main_splitter =
        MakeOwned<wxSplitterWindow>(this, wxID_ANY, wxDefaultPosition,
                                    wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    main_splitter_ = main_splitter;

    constexpr int kMinimumPaneSize = 5;
    constexpr double kSashGravity = 0.5;
    main_splitter->SetMinimumPaneSize(kMinimumPaneSize);
    main_splitter->SetSashGravity(kSashGravity);

    constexpr int kSizerBorder = 5;
    wxSizerFlags sizer_flags;
    sizer_flags.Proportion(1).Expand().Border(wxALL, kSizerBorder);

    auto* notebook_panel = MakeOwned<wxPanel>(main_splitter, wxID_ANY);
    auto* notebook_panel_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    notebook_panel->SetSizer(notebook_panel_sizer);

    auto* notebook = MakeOwned<wxNotebook>(
        notebook_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
    notebook_ = notebook;
    notebook_panel_sizer->Add(notebook, sizer_flags);
    CreatePanels(notebook);

    auto* gl_canvas_panel = MakeOwned<wxPanel>(main_splitter, wxID_ANY);
    auto* gl_canvas_panel_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    gl_canvas_panel->SetSizer(gl_canvas_panel_sizer);
    auto* gl_canvas = MakeOwned<GLCanvas>(gl_canvas_panel);
    gl_canvas_ = gl_canvas;
    gl_canvas_panel_sizer->Add(gl_canvas, sizer_flags);

    main_splitter->SplitVertically(notebook_panel, gl_canvas_panel);
  }

  void CreatePanels(wxNotebook* notebook) {
    data_table_panel_ =
        MakeOwned<DateTablePanel>(notebook, locale_date_formatter_);
    date_groups_table_panel_ = MakeOwned<DateGroupsTablePanel>(notebook);
    calendar_setup_panel_ = MakeOwned<CalendarSetupPanel>(notebook);
    scene_tree_panel_ = MakeOwned<SceneTreePanel>(notebook);

    // Page, font and title share the tab "Document"; the collecting panel owns
    // the three children, and they get wired one by one through the weak
    // references below.
    auto* document_setup_panel = MakeOwned<DocumentSetupPanel>(notebook);
    document_setup_panel_ = document_setup_panel;
    page_setup_panel_ = document_setup_panel->GetPageSetupPanel();
    font_panel_ = document_setup_panel->GetFontPanel();
    title_setup_panel_ = document_setup_panel->GetTitleSetupPanel();

    notebook->AddPage(date_groups_table_panel_, "Categories");
    notebook->AddPage(data_table_panel_, "Entries");
    notebook->AddPage(document_setup_panel, "Document");
    notebook->AddPage(calendar_setup_panel_, "Timeframe");
    notebook->AddPage(scene_tree_panel_, "Scene");
  }

  void InitMenu() {
    const MainMenuIds& ids = menu_.Ids();

    BindFileCommand(ids.open_xml, FileCommand::kOpenXml);
    BindFileCommand(ids.save_xml, FileCommand::kSaveXml);
    BindFileCommand(ids.save_as_xml, FileCommand::kSaveXmlAs);
    BindFileCommand(ids.import_csv, FileCommand::kImportCsv);
    BindFileCommand(ids.export_csv, FileCommand::kExportCsv);
    BindFileCommand(ids.export_png, FileCommand::kExportPng);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
    Bind(
        wxEVT_MENU,
        [this](wxCommandEvent&) {
          LicenseInformationDialog dialog(this);
          dialog.ShowModal();
        },
        ids.license_info);

    menu_.AttachTo(*this);
  }

  void BindFileCommand(int menu_id, FileCommand command) {
    Bind(
        wxEVT_MENU,
        [this, command](wxCommandEvent&) { signal_file_command_(command); },
        menu_id);
  }

  LocaleDateFormatter& locale_date_formatter_;

  wxWeakRef<wxSplitterWindow> main_splitter_;
  wxWeakRef<wxNotebook> notebook_;

  wxWeakRef<DateGroupsTablePanel> date_groups_table_panel_;
  wxWeakRef<DocumentSetupPanel> document_setup_panel_;
  wxWeakRef<PageSetupPanel> page_setup_panel_;
  wxWeakRef<TitleSetupPanel> title_setup_panel_;
  wxWeakRef<CalendarSetupPanel> calendar_setup_panel_;
  wxWeakRef<GLCanvas> gl_canvas_;
  wxWeakRef<FontPanel> font_panel_;
  wxWeakRef<DateTablePanel> data_table_panel_;
  wxWeakRef<SceneTreePanel> scene_tree_panel_;

  wxTimer exit_timer_;
  MainMenu menu_;
  sigslot::signal<FileCommand> signal_file_command_;
};

#endif  // MAIN_FRAME_HPP
