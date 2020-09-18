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

#include "gl_canvas.h"
#include "../calendar_page.h"
#include "../date_utils.h"
#include "date_groups_table.h"
#include "date_table_panel.h"
#include "page_setup.h"
#include "font_setup.h"
#include "title_setup.h"
#include "elements_setup.h"
#include "calendar_setup.h"
#include "license_info.h"


#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/platinfo.h>
#include <wx/activityindicator.h>
#include <wx/utils.h>
#include <wx/intl.h>

#include <csv/reader.hpp>
#include <csv/writer.hpp>

#include <pugixml.hpp>

#include <sigslot/signal.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <memory>


// Reason for use of raw pointers instead of smart_pointers:
// https://wiki.wxwidgets.org/Avoiding_Memory_Leaks 


class MainWindow : 
    public wxFrame
{
public:

    MainWindow(const wxString& title, const wxPoint& pos, const wxSize& size);

    GLCanvas* GetGLCanvas();
    
private:
    void InitMenu();

    void OpenGLReady();
    void EstablishConnections();

    void SlotLoadXML(wxCommandEvent& event);
    void SlotSaveXML(wxCommandEvent& event);
    void LoadXML(const std::wstring& filepath);
    void SaveXML(const std::wstring& filepath);
    
    void SlotImportCSV(wxCommandEvent& event);
    void SlotExportCSV(wxCommandEvent& event);
    void ImportCSV(const std::string& filepath);

    void SlotExportPNG(wxCommandEvent& event);

    void SlotExit(wxCommandEvent& event);

    void SlotLicenseInfo(wxCommandEvent& event);
    
    std::wstring current_xml_file_path;

    DateGroupsTablePanel* date_groups_table_panel;
    DateTablePanel* data_table_panel;
    CalendarSetupPanel* calendar_setup_panel;
    ElementsSetupsPanel* elements_setup_panel;
    PageSetupPanel* page_setup_panel;
    FontSetupPanel* font_setup_panel;
    TitleSetupPanel* title_setup_panel;
    

    std::unique_ptr<CalendarPage> calendar;

    DateGroupStore date_groups_store;
    DateIntervalBundleStore date_interval_bundle_store;
    TransformDateIntervalBundle transformed_date_interval_bundle;

    GLCanvas* glcanvas;

    const int ID_SAVE_XML;
    const int ID_SAVE_AS_XML;
};


class App : 
    public wxApp
{
public:

    App() :
        main_window(nullptr)
    {}

    bool OnInit() override;

private:

    MainWindow* main_window;

    std::unique_ptr<wxLocale> locale;
};
