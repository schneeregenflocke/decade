/*
Decade
Copyright (c) 2019-2025 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// #include <wx/setup.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/wx.h>

#include "calendar_page.hpp"
#include "date_utils.hpp"
#include "gui/calendar_panel.hpp"
#include "gui/date_panel.hpp"
#include "gui/font_panel.hpp"
#include "gui/groups_panel.hpp"
#include "gui/license_panel.hpp"
#include "gui/log_panel.hpp"
#include "gui/opengl_panel.hpp"
#include "gui/page_panel.hpp"
#include "gui/shape_panel.hpp"
#include "gui/title_panel.hpp"
#include "packages/shape_config.hpp"
#include "packages/title_config.hpp"
#include <array>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <csv2/reader.hpp>
#include <csv2/writer.hpp>
#include <fstream>
#include <gsl/gsl>
#include <iostream>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>

class MainWindow : public wxFrame {

public:
  MainWindow(wxWindow *parent, const wxString &title, const wxPoint &pos, const wxSize &size)
      : wxFrame(parent, wxID_ANY, title, pos, size), ID_SAVE_XML(wxWindow::NewControlId()),
        ID_SAVE_AS_XML(wxWindow::NewControlId())
  {
    std::cout << "C++ Standard: " << __cplusplus << '\n';

    this->Maximize();

    auto *main_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                               wxSP_3D | wxSP_LIVE_UPDATE);

    constexpr int minimum_pane_size = 5;
    constexpr double sash_gravity = 0.5;
    main_splitter->SetMinimumPaneSize(minimum_pane_size);
    main_splitter->SetSashGravity(sash_gravity);

    wxSizerFlags sizer_flags;
    sizer_flags.Proportion(1).Expand().Border(wxALL, 5);

    auto *notebook_panel = new wxPanel(main_splitter, wxID_ANY);
    auto forgrndColor = notebook_panel->GetForegroundColour();
    auto bckgrndColor = notebook_panel->GetBackgroundColour();

    auto *notebook_panel_sizer = new wxBoxSizer(wxVERTICAL);
    notebook_panel->SetSizer(notebook_panel_sizer);

    wxNotebook *notebook =
        new wxNotebook(notebook_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
    notebook_panel_sizer->Add(notebook, sizer_flags);

    log_panel = std::make_unique<LogPanel>(notebook);
    notebook->AddPage(log_panel->PanelPtr(), "Log");
    // std::cout.set_rdbuf(log_panel->GetTextCtrlPtr());
    // auto *cout_rdbuf = std::cout.rdbuf();

    data_table_panel = std::make_unique<DateTablePanel>(notebook);
    date_groups_table_panel = std::make_unique<DateGroupsTablePanel>(notebook);
    calendar_setup_panel = std::make_unique<CalendarSetupPanel>(notebook);
    elements_setup_panel = std::make_unique<ElementsSetupsPanel>(notebook);
    page_setup_panel = std::make_unique<PageSetupPanel>(notebook);
    font_panel = std::make_unique<FontPanel>(notebook);
    title_setup_panel = std::make_unique<TitleSetupPanel>(notebook);

    notebook->AddPage(data_table_panel->PanelPtr(), "Dates");
    notebook->AddPage(date_groups_table_panel->PanelPtr(), "Groups");
    notebook->AddPage(calendar_setup_panel->PanelPtr(), "Calendar");
    notebook->AddPage(elements_setup_panel->PanelPtr(), "Shapes");
    notebook->AddPage(page_setup_panel->PanelPtr(), "Page");
    notebook->AddPage(font_panel->PanelPtr(), "Font");
    notebook->AddPage(title_setup_panel->PanelPtr(), "Title");

    std::cout << std::string("__cplusplus ") + std::to_string(__cplusplus) << '\n';
    std::cout << "OperatingSystemIdName " << wxPlatformInfo::Get().GetOperatingSystemIdName()
              << '\n';
    std::cout << "ArchName " << wxPlatformInfo::Get().GetBitnessName() << '\n';
    std::cout << "OSMajorVersion.OSMinorVersion.OSMicroVersion "
              << wxPlatformInfo::Get().GetOSMajorVersion() << '.'
              << wxPlatformInfo::Get().GetOSMinorVersion() << '.'
              << wxPlatformInfo::Get().GetOSMicroVersion() << '\n';
    auto wxwidgets_version = std::wstring(wxVERSION_STRING);
    std::cout << "wxVERSION_STRING "
              << std::string(wxwidgets_version.cbegin(), wxwidgets_version.cend()) << '\n';

    wxPanel *gl_canvas_panel = new wxPanel(main_splitter, wxID_ANY);
    gl_canvas = std::make_unique<GLCanvas>(GLCanvas(gl_canvas_panel));
    wxBoxSizer *gl_canvas_panel_sizer = new wxBoxSizer(wxVERTICAL);
    gl_canvas_panel->SetSizer(gl_canvas_panel_sizer);
    gl_canvas_panel_sizer->Add(gl_canvas->GLCanvasPtr(), sizer_flags);

    main_splitter->SplitVertically(notebook_panel, gl_canvas_panel);

    InitMenu();
    CreateStatusBar(1);
    Show();
    Raise();

    std::array<int, 2> gl_version{4, 6};
    bool gl_loaded = gl_canvas->LoadOpenGL(gl_version) != 0;
    if (gl_loaded) {
      calendar_page =
          std::make_unique<CalendarPage>(gl_canvas.get(), font_panel->GetFontFilePath());
      EtablishConnections();
    }
  }

private:
  void EtablishConnections()
  {
    // Connections date_interval_bundle_store <-> data_table_panel
    date_interval_bundle_store.signal_date_interval_bundles.connect(
        &DateTablePanel::ReceiveDateIntervalBundles, data_table_panel.get());
    data_table_panel->signal_table_date_interval_bundles.connect(
        &DateIntervalBundleStore::ReceiveDateIntervalBundles, &date_interval_bundle_store);

    // Connect date_interval_bundle_store -> transform_date_interval_bundle ->
    // calendar
    date_interval_bundle_store.signal_date_interval_bundles.connect(
        &TransformDateIntervalBundle::ReceiveDateIntervalBundles, &transform_date_interval_bundle);
    transform_date_interval_bundle.SetTransform(0, 1);
    transform_date_interval_bundle.signal_transform_date_interval_bundles.connect(
        &CalendarPage::ReceiveDateIntervalBundles, calendar_page.get());

    // Connections date_groups_store <-> date_groups_panel
    date_groups_store.signal_date_groups.connect(&DateGroupsTablePanel::ReceiveDateGroups,
                                                 date_groups_table_panel.get());
    date_groups_table_panel->signal_table_date_groups.connect(&DateGroupStore::ReceiveDateGroups,
                                                              &date_groups_store);

    // Connections date_groups_store -> ...
    date_groups_store.signal_date_groups.connect(&DateIntervalBundleStore::ReceiveDateGroups,
                                                 &date_interval_bundle_store);
    date_groups_store.signal_date_groups.connect(&DateTablePanel::ReceiveDateGroups,
                                                 data_table_panel.get());
    date_groups_store.signal_date_groups.connect(&ElementsSetupsPanel::ReceiveDateGroups,
                                                 elements_setup_panel.get());
    date_groups_store.signal_date_groups.connect(&CalendarPage::ReceiveDateGroups,
                                                 calendar_page.get());

    // Connections page_setup_store <-> page_setup_panel
    page_setup_store.signal_page_setup_config.connect(&PageSetupPanel::ReceivePageSetup,
                                                      page_setup_panel.get());
    page_setup_panel->signal_page_setup_config.connect(&PageSetupStore::ReceivePageSetup,
                                                       &page_setup_store);

    // Connections page_setup -> ...
    page_setup_store.signal_page_setup_config.connect(&CalendarPage::ReceivePageSetup,
                                                      calendar_page.get());
    page_setup_store.signal_page_setup_config.connect(&GLCanvas::ReceivePageSetup, gl_canvas.get());

    // Connections font_panel -> ...
    font_panel->signal_font_filepath.connect(&CalendarPage::ReceiveFont, calendar_page.get());

    // Connections title_config_store <-> title_setup_panel
    title_config_store.signal_title_config.connect(&TitleSetupPanel::ReceiveTitleConfig,
                                                   title_setup_panel.get());
    title_setup_panel->signal_title_config.connect(&TitleConfigStore::ReceiveTitleConfig,
                                                   &title_config_store);

    // Connections title_config_store -> ...
    title_config_store.signal_title_config.connect(&CalendarPage::ReceiveTitleConfig,
                                                   calendar_page.get());

    // Connections title_config_store <-> elements_setup_panel
    shape_configuration_storage.signal_shape_configuration_storage.connect(
        &ElementsSetupsPanel::ReceiveShapeConfigurationStorage, elements_setup_panel.get());
    elements_setup_panel->signal_shape_configuration_storage.connect(
        &ShapeConfigurationStorage::ReceiveShapeConfigurationStorage, &shape_configuration_storage);

    // Connections elements_setup_panel -> ...
    shape_configuration_storage.signal_shape_configuration_storage.connect(
        &CalendarPage::ReceiveShapeConfigurationStorage, calendar_page.get());

    // Connections calendar_configuration_storage <-> calendar_panel
    calendar_configuration_storage.signal_calendar_config_storage.connect(
        &CalendarSetupPanel::ReceiveCalendarConfigStorage, calendar_setup_panel.get());
    calendar_setup_panel->signal_calendar_config_storage.connect(
        &CalendarConfigStorage::ReceiveCalendarConfigStorage, &calendar_configuration_storage);

    // Connections calendar_configuration_storage -> ...
    calendar_configuration_storage.signal_calendar_config_storage.connect(
        &CalendarPage::ReceiveCalendarConfig, calendar_page.get());

    shape_configuration_storage.SendShapeConfigurationStorage();
    date_groups_store.SendDefaultValues();
    page_setup_panel->SendDefaultValues();
    title_setup_panel->SendDefaultValues();
    calendar_configuration_storage.SendCalendarConfigStorage();
  }

  void InitMenu()
  {
    const int ID_EXPORT_PNG = wxWindow::NewControlId();
    const int ID_IMPORT_CSV = wxWindow::NewControlId();
    const int ID_EXPORT_CSV = wxWindow::NewControlId();
    const int ID_OPEN_XML = wxWindow::NewControlId();
    const int ID_LICENSE_INFO = wxWindow::NewControlId();
    // const int ID_NEW_XML = wxWindow::NewControlId();
    // const int ID_CLOSE_XML = wxWindow::NewControlId();

    this->Bind(wxEVT_MENU, &MainWindow::CallbackLoadXML, this, ID_OPEN_XML);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackSaveXML, this, ID_SAVE_XML);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackSaveXML, this, ID_SAVE_AS_XML);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackImportCSV, this, ID_IMPORT_CSV);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackExportCSV, this, ID_EXPORT_CSV);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackExportPNG, this, ID_EXPORT_PNG);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackExit, this, wxID_EXIT);
    this->Bind(wxEVT_MENU, &MainWindow::CallbackLicenseInfo, this, ID_LICENSE_INFO);

    wxMenuBar *menu_bar = new wxMenuBar;
    this->SetMenuBar(menu_bar);

    wxMenu *menu_file = new wxMenu;
    menu_bar->Append(menu_file, "&File");

    menu_file->Append(ID_OPEN_XML, L"&Open...");
    menu_file->AppendSeparator();
    menu_file->Append(ID_SAVE_XML, L"&Save \tCTRL+S");
    menu_file->Append(ID_SAVE_AS_XML, L"&Save As...");
    menu_file->AppendSeparator();
    menu_file->Append(ID_IMPORT_CSV, L"&Import csv...");
    menu_file->Append(ID_EXPORT_CSV, L"&Export csv...");
    menu_file->AppendSeparator();
    menu_file->Append(ID_EXPORT_PNG, L"&Export png (600 dpi)...");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_EXIT);

    wxMenu *menu_help = new wxMenu;
    menu_bar->Append(menu_help, "&Help");
    menu_help->Append(ID_LICENSE_INFO, L"&Open Source Licenses");
  }

  void CallbackLoadXML(wxCommandEvent &event)
  {
    wxFileDialog openFileDialog(this, "Open File", wxEmptyString, wxEmptyString,
                                "XML Files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_OK) {
      std::string file_path = openFileDialog.GetPath().ToStdString();
      LoadXML(file_path);
      xml_file_path = file_path;
    }
  }

  void CallbackSaveXML(wxCommandEvent &event)
  {
    if (event.GetId() == ID_SAVE_XML && xml_file_path.empty() == false) {
      SaveXML(xml_file_path);
    }

    else if (event.GetId() == ID_SAVE_AS_XML || xml_file_path.empty() == true) {
      wxFileDialog saveFileDialog(this, "Save File", wxEmptyString, wxEmptyString,
                                  "XML Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if (saveFileDialog.ShowModal() == wxID_OK) {
        std::string file_path = saveFileDialog.GetPath().ToStdString();
        SaveXML(file_path);
        xml_file_path = file_path;
      }
    }
  }

  void LoadXML(const std::string &filepath)
  {
    std::ifstream filestream(filepath);
    boost::archive::xml_iarchive iarchive(filestream);

    iarchive >> BOOST_SERIALIZATION_NVP(date_groups_store);
    iarchive >> BOOST_SERIALIZATION_NVP(date_interval_bundle_store);
    iarchive >> BOOST_SERIALIZATION_NVP(page_setup_store);
    iarchive >> BOOST_SERIALIZATION_NVP(title_config_store);
    iarchive >> BOOST_SERIALIZATION_NVP(shape_configuration_storage);
    iarchive >> BOOST_SERIALIZATION_NVP(calendar_configuration_storage);
  }

  void SaveXML(const std::string &filepath)
  {
    std::ofstream filestream(filepath);
    boost::archive::xml_oarchive oarchive(filestream);

    oarchive << BOOST_SERIALIZATION_NVP(date_groups_store);
    oarchive << BOOST_SERIALIZATION_NVP(date_interval_bundle_store);
    oarchive << BOOST_SERIALIZATION_NVP(page_setup_store);
    oarchive << BOOST_SERIALIZATION_NVP(title_config_store);
    oarchive << BOOST_SERIALIZATION_NVP(shape_configuration_storage);
    oarchive << BOOST_SERIALIZATION_NVP(calendar_configuration_storage);
  }

  void CallbackImportCSV(wxCommandEvent &event)
  {
    wxFileDialog open_file_dialog(this, "Import file", wxEmptyString, wxEmptyString,
                                  "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
                                  wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (open_file_dialog.ShowModal() == wxID_OK) {
      std::string file_path = open_file_dialog.GetPath().ToStdString();
      csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>,
                   csv2::first_row_is_header<false>, csv2::trim_policy::trim_whitespace>
          csv_reader;
      csv_reader.mmap(file_path);
      date_format_descriptor date_format = InitDateFormat();
      std::vector<DateIntervalBundle> date_interval_bundles;

      if (csv_reader.mmap(file_path)) {
        for (const auto row : csv_reader) {
          size_t current_col = 0;
          std::string begin_date_string;
          std::string end_date_string;

          for (const auto cell : row) {
            if (current_col == 0) {
              cell.read_value(begin_date_string);
            }
            if (current_col == 1) {
              cell.read_value(end_date_string);
            }
            ++current_col;
          }

          const auto begin_date = string_to_boost_date(begin_date_string, date_format);
          const auto end_date = string_to_boost_date(end_date_string, date_format);

          DateIntervalBundle date_interval_bundle;
          date_interval_bundle.date_interval = boost::gregorian::date_period(begin_date, end_date);
          date_interval_bundles.push_back(date_interval_bundle);
        }
      }
      date_interval_bundle_store.ReceiveDateIntervalBundles(date_interval_bundles);
    }
  }

  void CallbackExportCSV(wxCommandEvent &event)
  {
    wxFileDialog save_file_dialog(this, "Export file", wxEmptyString, wxEmptyString,
                                  "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
                                  wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (save_file_dialog.ShowModal() == wxID_OK) {
      const std::string file_path = save_file_dialog.GetPath().ToStdString();
      std::ofstream file_stream(file_path, std::ios_base::trunc);
      bool file_open = file_stream.is_open();
      csv2::Writer<csv2::delimiter<','>> csv_writer(file_stream);
      const auto date_interval_bundles = date_interval_bundle_store.GetDateIntervalBundles();

      for (size_t index = 0; index < date_interval_bundles.size(); ++index) {
        std::array<std::string, 2> date_interval_strings;
        date_interval_strings[0] =
            boost_date_to_string(date_interval_bundles[index].date_interval.begin());
        date_interval_strings[1] =
            boost_date_to_string(date_interval_bundles[index].date_interval.end());
        csv_writer.write_row(date_interval_strings);
      }

      file_stream.close();
    }
  }

  void CallbackExportPNG(wxCommandEvent &event)
  {
    wxFileDialog file_dialog(this, "Export PNG file", wxEmptyString, wxEmptyString,
                             "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (file_dialog.ShowModal() == wxID_OK) {
      std::string file_path = file_dialog.GetPath().ToStdString();
      gl_canvas->SavePNG(file_path);
    }
  }

  void CallbackExit(wxCommandEvent &event) { Close(true); }

  void CallbackLicenseInfo(wxCommandEvent &event)
  {
    LicenseInformationDialog dialog;
    dialog.ShowModal();
  }

  std::string xml_file_path;

  // wxWidgets Panels
  std::unique_ptr<DateGroupsTablePanel> date_groups_table_panel;
  std::unique_ptr<ElementsSetupsPanel> elements_setup_panel;
  std::unique_ptr<PageSetupPanel> page_setup_panel;
  std::unique_ptr<TitleSetupPanel> title_setup_panel;
  std::unique_ptr<CalendarSetupPanel> calendar_setup_panel;
  std::unique_ptr<GLCanvas> gl_canvas;
  std::unique_ptr<FontPanel> font_panel;
  std::unique_ptr<LogPanel> log_panel;
  std::unique_ptr<DateTablePanel> data_table_panel;

  // Storages
  DateGroupStore date_groups_store;
  DateIntervalBundleStore date_interval_bundle_store;
  TransformDateIntervalBundle transform_date_interval_bundle;
  PageSetupStore page_setup_store;
  TitleConfigStore title_config_store;
  ShapeConfigurationStorage shape_configuration_storage;
  CalendarConfigStorage calendar_configuration_storage;

  // Calendar Page
  std::unique_ptr<CalendarPage> calendar_page;

  // wxWidgets Controller IDs
  const int ID_SAVE_XML;
  const int ID_SAVE_AS_XML;
};
