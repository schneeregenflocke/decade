#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/string.h>
#include <wx/window.h>

#include <memory>

// Command identifiers for the application's main menu. Generated once with
// wxWindow::NewControlId so they never clash with framework-reserved IDs. The
// frame binds its callbacks against these IDs.
struct MainMenuIds {
  int open_xml{wxWindow::NewControlId()};
  int save_xml{wxWindow::NewControlId()};
  int save_as_xml{wxWindow::NewControlId()};
  int import_csv{wxWindow::NewControlId()};
  int export_csv{wxWindow::NewControlId()};
  int export_png{wxWindow::NewControlId()};
  int license_info{wxWindow::NewControlId()};
};

// Presentation layer: owns the main menu *layout* only (File + Help). Command
// handling stays in the frame; this class just builds the wxMenuBar, attaches
// it to a frame, and exposes the command IDs the frame binds against. Splitting
// it out of MainWindow follows SRP — the menu changes when the menu layout
// changes, independent of the window's wiring and callbacks.
class MainMenu {
 public:
  explicit MainMenu(int export_png_dpi) : export_png_dpi_(export_png_dpi) {}

  [[nodiscard]] const MainMenuIds& Ids() const { return ids_; }

  void AttachTo(wxFrame& frame) const {
    auto menu_bar = std::make_unique<wxMenuBar>();
    auto* menu_bar_ptr = menu_bar.release();
    frame.SetMenuBar(menu_bar_ptr);

    auto menu_file = std::make_unique<wxMenu>();
    auto* menu_file_ptr = menu_file.release();
    menu_bar_ptr->Append(menu_file_ptr, "&File");
    menu_file_ptr->Append(ids_.open_xml, L"&Open...");
    menu_file_ptr->AppendSeparator();
    menu_file_ptr->Append(ids_.save_xml, L"&Save \tCTRL+S");
    menu_file_ptr->Append(ids_.save_as_xml, L"&Save As...");
    menu_file_ptr->AppendSeparator();
    menu_file_ptr->Append(ids_.import_csv, L"&Import csv...");
    menu_file_ptr->Append(ids_.export_csv, L"&Export csv...");
    menu_file_ptr->AppendSeparator();
    menu_file_ptr->Append(ids_.export_png, ExportPngLabel());
    menu_file_ptr->AppendSeparator();
    menu_file_ptr->Append(wxID_EXIT);

    auto menu_help = std::make_unique<wxMenu>();
    auto* menu_help_ptr = menu_help.release();
    menu_bar_ptr->Append(menu_help_ptr, "&Help");
    menu_help_ptr->Append(ids_.license_info, L"&Open Source Licenses");
  }

 private:
  [[nodiscard]] wxString ExportPngLabel() const {
    return wxString::Format("&Export png (%d dpi)...", export_png_dpi_);
  }

  MainMenuIds ids_;
  int export_png_dpi_;
};

#endif  // MAIN_MENU_HPP
