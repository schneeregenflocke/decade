/*
Decade
Copyright (c) 2019-2020 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/


#pragma once

#include "calendar_page.h"
#include "date_utils.h"

#include "gui/wx_widgets_include.h"

#include "gui/gl_canvas.h"
#include "gui/date_groups_table.h"
#include "gui/date_table_panel.h"
#include "gui/page_setup_panel.h"
#include "gui/font_setup.h"
#include "gui/title_setup_panel.h"
#include "gui/elements_setup.h"
#include "gui/calendar_setup.h"
#include "gui/license_info.h"


#include <sigslot/signal.hpp>

#include <pugixml.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <csv/reader.hpp>
#include <csv/writer.hpp>


#include <iostream>
#include <string>
#include <cmath>
#include <memory>


class MainWindow :
    public wxFrame
{
public:

    MainWindow(const wxString& title, const wxPoint& pos, const wxSize& size) :
        wxFrame(nullptr, wxID_ANY, title, pos, size),
        current_xml_file_path(""),
        ID_SAVE_XML(NewControlId()),
        ID_SAVE_AS_XML(NewControlId())
    {
        ////////////////////////////////////////////////////////////////////////////////

        wxSplitterWindow* main_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxCLIP_CHILDREN);
        main_splitter->SetMinimumPaneSize(20);
        main_splitter->SetSashGravity(0.25);

        ////////////////////////////////////////////////////////////////////////////////

        wxSizerFlags sizer_flags;
        sizer_flags.Proportion(1).Expand().Border(wxALL, 5);

        ////////////////////////////////////////////////////////////////////////////////

        wxPanel* notebook_panel = new wxPanel(main_splitter, wxID_ANY);

        wxBoxSizer* notebook_panel_sizer = new wxBoxSizer(wxVERTICAL);
        notebook_panel->SetSizer(notebook_panel_sizer);

        wxNotebook* notebook = new wxNotebook(notebook_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
        notebook_panel_sizer->Add(notebook, sizer_flags);

        ////////////////////////////////////////////////////////////////////////////////

        data_table_panel = new DateTablePanel(notebook);
        date_groups_table_panel = new DateGroupsTablePanel(notebook);
        calendar_setup_panel = new CalendarSetupPanel(notebook);
        elements_setup_panel = new ElementsSetupsPanel(notebook);
        page_setup_panel = new PageSetupPanel(notebook);
        font_setup_panel = new FontSetupPanel(notebook);
        title_setup_panel = new TitleSetupPanel(notebook);

        notebook->AddPage(data_table_panel, "Date Table");
        notebook->AddPage(date_groups_table_panel, "Date Groups");
        notebook->AddPage(calendar_setup_panel, "Calendar Setup");
        notebook->AddPage(elements_setup_panel, "Elements Setup");
        notebook->AddPage(page_setup_panel, "Page Setup");
        notebook->AddPage(font_setup_panel, "Font Setup");
        notebook->AddPage(title_setup_panel, "Title Setup");

        ////////////////////////////////////////////////////////////////////////////////

        wxGLAttributes attributes;
        attributes.PlatformDefaults().Defaults().EndList();
        bool display_supported = wxGLCanvas::IsDisplaySupported(attributes);
        std::cout << "wxGLCanvas IsDisplaySupported " << std::boolalpha << display_supported << '\n';

        wxPanel* glcanvas_panel = new wxPanel(main_splitter, wxID_ANY);
        wxBoxSizer* glcanvas_panel_sizer = new wxBoxSizer(wxVERTICAL);
        glcanvas_panel->SetSizer(glcanvas_panel_sizer);
        glcanvas = new GLCanvas(glcanvas_panel, attributes);
        glcanvas_panel_sizer->Add(glcanvas, sizer_flags);

        glcanvas->signal_opengl_ready.connect(&MainWindow::OpenGLReady, this);

        ////////////////////////////////////////////////////////////////////////////////

        main_splitter->SplitVertically(notebook_panel, glcanvas_panel);

        //Layout();

        ////////////////////////////////////////////////////////////////////////////////

        InitMenu();

        CreateStatusBar(1);
    }

    GLCanvas* GetGLCanvas()
    {
        return glcanvas;
    }

private:
    void InitMenu()
    {
        const int ID_EXPORT_PNG = NewControlId();
        const int ID_IMPORT_CSV = NewControlId();
        const int ID_EXPORT_CSV = NewControlId();
        //const int ID_NEW_XML = wxNewId();
        //const int ID_CLOSE_XML = wxNewId();
        const int ID_OPEN_XML = NewControlId();
        const int ID_LICENSE_INFO = NewControlId();

        Bind(wxEVT_MENU, &MainWindow::SlotLoadXML, this, ID_OPEN_XML);
        Bind(wxEVT_MENU, &MainWindow::SlotSaveXML, this, ID_SAVE_XML);
        Bind(wxEVT_MENU, &MainWindow::SlotSaveXML, this, ID_SAVE_AS_XML);
        Bind(wxEVT_MENU, &MainWindow::SlotImportCSV, this, ID_IMPORT_CSV);
        Bind(wxEVT_MENU, &MainWindow::SlotExportCSV, this, ID_EXPORT_CSV);
        Bind(wxEVT_MENU, &MainWindow::SlotExportPNG, this, ID_EXPORT_PNG);
        Bind(wxEVT_MENU, &MainWindow::SlotExit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &MainWindow::SlotLicenseInfo, this, ID_LICENSE_INFO);

        wxMenuBar* menu_bar = new wxMenuBar;
        SetMenuBar(menu_bar);

        wxMenu* menu_file = new wxMenu;
        menu_bar->Append(menu_file, "&File");

        menu_file->Append(ID_OPEN_XML, L"&Open...");
        menu_file->AppendSeparator();
        menu_file->Append(ID_SAVE_XML, L"&Save");
        menu_file->Append(ID_SAVE_AS_XML, L"&Save As...");
        menu_file->AppendSeparator();
        menu_file->Append(ID_IMPORT_CSV, L"&Append csv...");
        menu_file->Append(ID_EXPORT_CSV, L"&Export csv...");
        menu_file->AppendSeparator();
        menu_file->Append(ID_EXPORT_PNG, L"&Export png...");
        menu_file->AppendSeparator();
        menu_file->Append(wxID_EXIT);

        wxMenu* menu_help = new wxMenu;
        menu_bar->Append(menu_help, "&Help");
        menu_help->Append(ID_LICENSE_INFO, L"&Open Source Licenses");
    }

    void OpenGLReady()
    {
        calendar = std::make_unique<CalendarPage>(glcanvas->GetGraphicEngine());
        calendar->Update();

        EstablishConnections();
    }
    void EstablishConnections()
    {
        // Connections date_interval_bundle_store <-> data_table_panel
        date_interval_bundle_store.signal_date_interval_bundles.connect(&DateTablePanel::ReceiveDateIntervalBundles, data_table_panel);
        data_table_panel->signal_table_date_interval_bundles.connect(&DateIntervalBundleStore::ReceiveDateIntervalBundles, &date_interval_bundle_store);

        // Connect date_interval_bundle_store -> transform_date_interval_bundle -> calendar
        date_interval_bundle_store.signal_date_interval_bundles.connect(&TransformDateIntervalBundle::ReceiveDateIntervalBundles, &transform_date_interval_bundle);
        transform_date_interval_bundle.SetTransform(0, 1);
        transform_date_interval_bundle.signal_transform_date_interval_bundles.connect(&CalendarPage::ReceiveDateIntervalBundles, calendar.get());

        // Connections date_groups_store <-> date_groups_panel
        date_groups_store.signal_date_groups.connect(&DateGroupsTablePanel::ReceiveDateGroups, date_groups_table_panel);
        date_groups_table_panel->signal_table_date_groups.connect(&DateGroupStore::ReceiveDateGroups, &date_groups_store);

        // Connections date_groups_store -> ...
        date_groups_store.signal_date_groups.connect(&DateIntervalBundleStore::ReceiveDateGroups, &date_interval_bundle_store);
        date_groups_store.signal_date_groups.connect(&DateTablePanel::ReceiveDateGroups, data_table_panel);
        date_groups_store.signal_date_groups.connect(&ElementsSetupsPanel::ReceiveDateGroups, elements_setup_panel);
        date_groups_store.signal_date_groups.connect(&CalendarPage::ReceiveDateGroups, calendar.get());

        // Connections page_setup_store <-> page_setup_panel
        page_setup_store.signal_page_setup_config.connect(&PageSetupPanel::ReceivePageSetup, page_setup_panel);
        page_setup_panel->signal_page_setup_config.connect(&PageSetupStore::ReceivePageSetup, &page_setup_store);

        // Connections page_setup -> ...
        page_setup_store.signal_page_setup_config.connect(&CalendarPage::ReceivePageSetup, calendar.get());
        page_setup_store.signal_page_setup_config.connect(&GraphicEngine::ReceivePageSetup, glcanvas->GetGraphicEngine());

        // Connections font_setup_panel -> ...
        font_setup_panel->signal_font_file_path.connect(&CalendarPage::ReceiveFont, calendar.get());

        // Connections title_setup_panel -> ...
        title_setup_panel->signal_frame_height.connect(&CalendarPage::ReceiveTitleFrameHeight, calendar.get());
        title_setup_panel->signal_font_size_ratio.connect(&CalendarPage::ReceiveTitleFontSizeRatio, calendar.get());
        title_setup_panel->signal_title_text.connect(&CalendarPage::ReceiveTitleText, calendar.get());
        title_setup_panel->signal_text_color.connect(&CalendarPage::ReceiveTitleTextColor, calendar.get());

        // Connections elements_setup_panel -> ...
        elements_setup_panel->signal_shape_config.connect(&CalendarPage::ReceiveRectangleShapeConfig, calendar.get());

        // Connections calendar_setup_panel -> ...
        calendar_setup_panel->signal_calendar_config.connect(&CalendarPage::ReceiveCalendarConfig, calendar.get());

        // Send Default Values
        date_groups_store.SendDefaultValues();
        page_setup_panel->SendDefaultValues();
        font_setup_panel->SendDefaultValues();
        title_setup_panel->SendDefaultValues();
        elements_setup_panel->SendDefaultValues();
        calendar_setup_panel->SendDefaultValues();
    }

    void SlotLoadXML(wxCommandEvent& event)
    {
        wxFileDialog openFileDialog(this, "Open File", wxEmptyString, wxEmptyString, "XML Files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (openFileDialog.ShowModal() == wxID_OK)
        {
            std::string file_path = openFileDialog.GetPath().ToStdString();
            LoadXML(file_path);
            current_xml_file_path = file_path;
        }


    }
    void SlotSaveXML(wxCommandEvent& event)
    {
        if (event.GetId() == ID_SAVE_XML && current_xml_file_path.empty() == false)
        {
            SaveXML(current_xml_file_path);
        }
        else if (event.GetId() == ID_SAVE_AS_XML || current_xml_file_path.empty() == true)
        {
            wxFileDialog saveFileDialog(this, "Save File", wxEmptyString, wxEmptyString, "XML Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

            if (saveFileDialog.ShowModal() == wxID_OK)
            {
                std::string file_path = saveFileDialog.GetPath().ToStdString();
                SaveXML(file_path);
                current_xml_file_path = file_path;
            }
        }
    }
    void LoadXML(const std::string& filepath)
    {

        std::ifstream filestream("testfile.txt");
        boost::archive::text_iarchive oarchive(filestream);
        oarchive >> date_groups_store;
        oarchive >> date_interval_bundle_store;
        oarchive >> page_setup_store;


        pugi::xml_document doc;
        auto load_success = doc.load_file(filepath.c_str());
        if (load_success)
        {
            //date_groups_store.LoadXML(doc);
            //date_interval_bundle_store.LoadXML(doc);
            //page_setup_panel->LoadXML(doc);
            //
            //title_setup_panel->LoadFromXML(doc);
            elements_setup_panel->LoadFromXML(doc);
            calendar_setup_panel->LoadXML(doc);
        }
    }
    void SaveXML(const std::string& filepath)
    {

        std::ofstream filestream("testfile.txt");
        boost::archive::text_oarchive oarchive(filestream);
        oarchive << date_groups_store;
        oarchive << date_interval_bundle_store;
        oarchive << page_setup_store;


        pugi::xml_document doc;

        //date_groups_store.SaveXML(&doc);
        //date_interval_bundle_store.SaveXML(&doc);
        //page_setup_panel->SaveXML(&doc);
        //
        //title_setup_panel->SaveToXML(&doc);
        elements_setup_panel->SaveToXML(&doc);
        calendar_setup_panel->SaveXML(&doc);

        doc.save_file(filepath.c_str());
    }

    const int ID_SAVE_XML;
    const int ID_SAVE_AS_XML;

    void SlotImportCSV(wxCommandEvent& event)
    {
        wxFileDialog open_file_dialog(this, "Import file", wxEmptyString, wxEmptyString, "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (open_file_dialog.ShowModal() == wxID_OK)
        {
            std::string file_path = open_file_dialog.GetPath().ToStdString();

            csv::Reader csv_reader;
            csv_reader.configure_dialect("custom_dialect")
                .delimiter(",")
                .header(false)
                .skip_empty_rows(true);

            csv_reader.read(file_path);

            date_format_descriptor date_format = InitDateFormat();

            std::vector<DateIntervalBundle> date_interval_bundles = date_interval_bundle_store.GetDateIntervalBundles();

            while (csv_reader.busy())
            {
                if (csv_reader.ready())
                {
                    auto row = csv_reader.next_row(); // auto& row = csv_reader.next_row();

                    const auto begin_date_string = row[std::to_string(0)];
                    const auto end_date_string = row[std::to_string(1)];

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
    void SlotExportCSV(wxCommandEvent& event)
    {
        wxFileDialog save_file_dialog(this, "Export file", wxEmptyString, wxEmptyString, "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (save_file_dialog.ShowModal() == wxID_OK)
        {
            const std::string file_path = save_file_dialog.GetPath().ToStdString();

            csv::Writer csv_writer(file_path.c_str());

            csv_writer.configure_dialect("custom_dialect")
                .delimiter(",")
                .header(false)
                .skip_empty_rows(true);

            const auto date_interval_bundles = date_interval_bundle_store.GetDateIntervalBundles();

            for (size_t index = 0; index < date_interval_bundles.size(); ++index)
            {
                const auto begin_date = boost_date_to_string(date_interval_bundles[index].date_interval.begin());
                const auto last_date = boost_date_to_string(date_interval_bundles[index].date_interval.end());
                csv_writer.write_row(begin_date, last_date);
            }

            csv_writer.close();
        }
    }
    void SlotExportPNG(wxCommandEvent& event)
    {
        wxFileDialog file_dialog(this, "Export PNG file", wxEmptyString, wxEmptyString, "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (file_dialog.ShowModal() == wxID_OK)
        {
            std::wstring file_path = file_dialog.GetPath().ToStdWstring();
            glcanvas->GetGraphicEngine()->RenderToPNG(file_path);
        }
    }
    void SlotExit(wxCommandEvent& event)
    {
        Close(true);
    }
    void SlotLicenseInfo(wxCommandEvent& event)
    {
        LicenseInformationDialog dialog;
        dialog.ShowModal();
    }

    std::string current_xml_file_path;

    // wxWidgets Panels
    DateGroupsTablePanel* date_groups_table_panel;
    DateTablePanel* data_table_panel;
    CalendarSetupPanel* calendar_setup_panel;
    ElementsSetupsPanel* elements_setup_panel;
    PageSetupPanel* page_setup_panel;
    FontSetupPanel* font_setup_panel;
    TitleSetupPanel* title_setup_panel;
    GLCanvas* glcanvas;

    // 
    std::unique_ptr<CalendarPage> calendar;

    DateGroupStore date_groups_store;
    DateIntervalBundleStore date_interval_bundle_store;
    TransformDateIntervalBundle transform_date_interval_bundle;

    PageSetupStore page_setup_store;
};
