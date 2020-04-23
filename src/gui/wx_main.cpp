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


#include "wx_main.h"


#ifdef _WIN32

extern "C" 
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

#endif


wxIMPLEMENT_APP_NO_MAIN(App);


int main(int argc, char* argv[])
{
    std::wcout << "__cplusplus " + std::to_string(__cplusplus) + '\n';

#if defined _WIN32
    std::wcout << "_WIN32 " << _WIN32 << '\n';
#endif

#if defined _MSC_VER && _MSVC_LANG
    std::wcout << "_MSC_VER " << _MSC_VER << '\n';
    std::wcout << "_MSVC_LANG " << _MSVC_LANG << '\n';
#endif

    wxEntryStart(argc, argv);

    wxTheApp->CallOnInit();
    wxTheApp->OnRun();
    wxTheApp->OnExit();
    
    wxEntryCleanup();

    return EXIT_SUCCESS;
}


bool App::OnInit()
{
    std::locale::global(std::locale(""));
    std::wcout << "current locale name " << std::locale("").name() << '\n';

    std::wcout << "OperatingSystemIdName " << wxPlatformInfo::Get().GetOperatingSystemIdName() << '\n';
    std::wcout << "ArchName " << wxPlatformInfo::Get().GetArchName() << '\n';

    std::wcout << "OSMajorVersion.OSMinorVersion.OSMicroVersion " << wxPlatformInfo::Get().GetOSMajorVersion() << '.' <<
        wxPlatformInfo::Get().GetOSMinorVersion() << '.' <<
        wxPlatformInfo::Get().GetOSMicroVersion() << '\n';

    std::wcout << "wxVERSION_STRING " << wxVERSION_STRING << '\n';

    ////////////////////////////////////////////////////////////////////////////////
    
    main_window = new MainWindow("Decade", wxPoint(100, 100), wxSize(800, 600));
    //main_window->Maximize();
    main_window->Show();
    main_window->Raise();

    std::array<int, 2> gl_version{ 3, 2 };
    
    main_window->GetGLCanvas()->LoadOpenGL(gl_version);

    ////////////////////////////////////////////////////////////////////////////////

    std::wcout << "ContentScaleFactor " << main_window->GetContentScaleFactor() << '\n';

    return true;
}


MainWindow::MainWindow(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(nullptr, wxID_ANY, title, pos, size),
    current_xml_file(L""),
    ID_SAVE_XML(NewControlId()),
    ID_SAVE_AS_XML(NewControlId())
{
    wxSplitterWindow* main_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxCLIP_CHILDREN);
    main_splitter->SetMinimumPaneSize(20); 
    main_splitter->SetSashGravity(0.25);

    ////////////////////////////////////////////////////////////////////////////////

    wxSplitterWindow* list_book_splitter;
    list_book_splitter = new wxSplitterWindow(main_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxCLIP_CHILDREN);
    list_book_splitter->SetMinimumPaneSize(20);
    list_book_splitter->SetSashGravity(0.25);

    wxPanel* list_panel;
    list_panel = new wxPanel(list_book_splitter, wxID_ANY);

    wxListBox* list_box = new wxListBox(list_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB, wxDefaultValidator, wxListBoxNameStr);
    
    wxBoxSizer* list_panel_sizer = new wxBoxSizer(wxVERTICAL);
    list_panel_sizer->Add(list_box, 1, wxEXPAND | wxALL, 5);
    list_panel->SetSizer(list_panel_sizer);

    ////////////////////////////////////////////////////////////////////////////////

    wxPanel* book_panel;
    book_panel = new wxPanel(list_book_splitter, wxID_ANY);

    ////////////////////////////////////////////////////////////////////////////////

    page_setup_panel = new PageSetupPanel(book_panel);
    data_table_panel = new DataTablePanel(book_panel);
    font_setup_panel = new FontSetupPanel(book_panel);
    title_setup_panel = new TitleSetupPanel(book_panel);
    elements_setup_panel = new ElementsSetupsPanel(book_panel);
    calendar_setup_panel = new CalendarSetupPanel(book_panel);

    list_box->AppendString(L"Page Setup");
    list_box->AppendString(L"Date Intervals Table");
    list_box->AppendString(L"Font Setup");
    list_box->AppendString(L"Title Setup");
    list_box->AppendString(L"Elements Setup");
    list_box->AppendString(L"Calendar Setup");


    book_panel_sizer = new wxBoxSizer(wxVERTICAL);
    book_panel->SetSizer(book_panel_sizer);

    book_panel_sizer->Add(page_setup_panel, 1, wxEXPAND | wxALL, 5);
    book_panel_sizer->Add(data_table_panel, 1, wxEXPAND | wxALL, 5);
    book_panel_sizer->Add(font_setup_panel, 1, wxEXPAND | wxALL, 5);
    book_panel_sizer->Add(title_setup_panel, 1, wxEXPAND | wxALL, 5);
    book_panel_sizer->Add(elements_setup_panel, 1, wxEXPAND | wxALL, 5);
    book_panel_sizer->Add(calendar_setup_panel, 1, wxEXPAND | wxALL, 5);

    book_panel_sizer->ShowItems(false);

    Bind(wxEVT_LISTBOX, &MainWindow::SlotSelectListBook, this);

    const size_t default_list_box_selection = 1;
    list_box->Select(default_list_box_selection);
    book_panel_sizer->Show(default_list_box_selection, true);
    book_panel_sizer->Layout();

    ////////////////////////////////////////////////////////////////////////////////

    list_book_splitter->SplitVertically(list_panel, book_panel);

    ////////////////////////////////////////////////////////////////////////////////
    
    wxGLAttributes attributes;
    attributes.PlatformDefaults().Defaults().EndList();
    bool display_supported = wxGLCanvas::IsDisplaySupported(attributes);
    std::cout << "wxGLCanvas IsDisplaySupported " << display_supported << '\n';

    wxPanel* gl_canvas_panel = new wxPanel(main_splitter, wxID_ANY);

    gl_canvas = new GLCanvas(gl_canvas_panel, attributes);

    wxBoxSizer* gl_canvas_panel_sizer = new wxBoxSizer(wxVERTICAL);
    gl_canvas_panel_sizer->Add(gl_canvas, 1, wxEXPAND | wxALL, 5);
    gl_canvas_panel->SetSizer(gl_canvas_panel_sizer);

    //Bind(gl_canvas->GetGLReadyEventTag(), &MainWindow::SlotGLReady, this);
    gl_canvas->signal_opengl_ready.connect(&MainWindow::SlotGLReady, this);
    ////////////////////////////////////////////////////////////////////////////////

    main_splitter->SplitVertically(list_book_splitter, gl_canvas_panel);

    ////////////////////////////////////////////////////////////////////////////////

    InitMenu();

    ////////////////////////////////////////////////////////////////////////////////

    CreateStatusBar(1);
}


GLCanvas* MainWindow::GetGLCanvas()
{
    return gl_canvas;
}


void MainWindow::SlotSelectListBook(wxCommandEvent& event)
{
    book_panel_sizer->ShowItems(false);
    
    auto selection = event.GetSelection();
    book_panel_sizer->Show(selection, true);

    book_panel_sizer->Layout();
}


void MainWindow::SlotGLReady()
{

    calendar = std::make_unique<CalendarPage>(gl_canvas->GetGraphicEngine());
    calendar->Update();


    dataStore.signal_date_intervals.connect(&DataTablePanel::SlotUpdateTable, data_table_panel);
    data_table_panel->signal_table_date_intervals.connect(&DateIntervals::SetDateIntervals, &dataStore);
    dataStore.SetTransform(0, 1);
    dataStore.signal_transformed_date_intervals.connect(&CalendarPage::SetDateIntervals, calendar.get());

    
    page_setup_panel->signal_page_size.connect(&CalendarPage::SlotPageSize, calendar.get());
    page_setup_panel->signal_page_margins.connect(&CalendarPage::SlotPageMargins, calendar.get());
    page_setup_panel->signal_page_size.connect(&GraphicEngine::SlotPageSize, gl_canvas->GetGraphicEngine());
    page_setup_panel->SendDefaultValues();


    font_setup_panel->signal_font_file_path.connect(&CalendarPage::SlotSelectFont, calendar.get());
    font_setup_panel->SendDefaultValues();


    title_setup_panel->signal_frame_height.connect(&CalendarPage::SlotTitleFrameHeight, calendar.get());
    title_setup_panel->signal_font_size_ratio.connect(&CalendarPage::SlotTitleFontSizeRatio, calendar.get());
    title_setup_panel->signal_title_text.connect(&CalendarPage::SlotTitleText, calendar.get());
    title_setup_panel->signal_text_color.connect(&CalendarPage::SlotTitleTextColor, calendar.get());
    title_setup_panel->SendDefaultValues();


    elements_setup_panel->signal_shape_config.connect(&CalendarPage::SlotRectangleShapeConfig, calendar.get());
    elements_setup_panel->SendDefaultValues();


    calendar_setup_panel->ConnectSignalCalendarConfig(&CalendarPage::SlotCalendarConfig, calendar.get());
    calendar_setup_panel->SendDefaultValues();
}


void MainWindow::InitMenu()
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
    menu_file->Append(ID_IMPORT_CSV, L"&Import csv...");
    menu_file->Append(ID_EXPORT_CSV, L"&Export csv...");
    menu_file->AppendSeparator();
    menu_file->Append(ID_EXPORT_PNG, L"&Export png...");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_EXIT);

    wxMenu* menu_help = new wxMenu;
    menu_bar->Append(menu_help, "&Help");
    menu_help->Append(ID_LICENSE_INFO, L"&Open Source Licenses");
}

void MainWindow::SlotExit(wxCommandEvent& event)
{
    Close(true);
}

void MainWindow::SlotImportCSV(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Import file", wxEmptyString, wxEmptyString, "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_OK)
    {
        std::string filePath = openFileDialog.GetPath().ToStdString();

        ImportCSV(filePath);
    }
}

void MainWindow::SlotExportCSV(wxCommandEvent& event)
{
    wxFileDialog saveFileDialog(this, "Export file", wxEmptyString, wxEmptyString, "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_OK)
    {
        std::string filePath = saveFileDialog.GetPath().ToStdString();

        csv::Writer csv_writer(filePath.c_str());

        csv_writer.configure_dialect("custom_dialect")
            .delimiter("\t")
            .header(false)
            .skip_empty_rows(true);

        for(size_t index = 0; index < dataStore.GetDateIntervalsSize(); ++index)
        {
            auto begin_date = boost_date_to_string(dataStore.GetDateIntervalConstRef(index).begin());
            auto last_date = boost_date_to_string(dataStore.GetDateIntervalConstRef(index).end());
            
            csv_writer.write_row(begin_date, last_date);
        }

        csv_writer.close();
    }
}

void MainWindow::ImportCSV(const std::string& filepath)
{
    csv::Reader csv_reader;

    csv_reader.configure_dialect("custom_dialect")
        .delimiter("\t")
        .header(false)
        .skip_empty_rows(true);

    csv_reader.read(filepath);

    date_format_descriptor date_format = InitDateFormat();

    std::vector<date_period> date_intervals;
   
    while (csv_reader.busy())
    {
        if (csv_reader.ready())
        {
            ////////////////////////////////////////////////////////////////////////////////
            auto row = csv_reader.next_row(); // auto& row = csv_reader.next_row();

            auto begin_date = string_to_boost_date(row[std::to_string(0)], date_format);
            auto last_date = string_to_boost_date(row[std::to_string(1)], date_format);

            if ((begin_date.is_special() || last_date.is_special()) == false)
            {
                date_period date_interval(begin_date, last_date);

                if (date_interval.is_null() == false)
                {
                    date_intervals.push_back(date_interval);
                }
            }
        }
    }

    dataStore.SetDateIntervals(date_intervals);
}

void MainWindow::SlotExportPNG(wxCommandEvent& event)
{
    wxFileDialog file_dialog(this, "Export PNG file", wxEmptyString, wxEmptyString, "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (file_dialog.ShowModal() == wxID_OK)
    {
        std::wstring file_path = file_dialog.GetPath().ToStdWstring();
        gl_canvas->GetGraphicEngine()->RenderToPNG(file_path);
    }
}

void MainWindow::SlotLoadXML(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Open File", wxEmptyString, wxEmptyString, "XML Files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_OK)
    {
        std::wstring file_path = openFileDialog.GetPath().ToStdWstring();
        LoadXML(file_path);
        current_xml_file = file_path;
    }   
}

void MainWindow::SlotSaveXML(wxCommandEvent& event)
{
    if (event.GetId() == ID_SAVE_XML && current_xml_file != std::wstring(L""))
    {
        SaveXML(current_xml_file);
    }
    else
    {
        wxFileDialog saveFileDialog(this, "Save File", wxEmptyString, wxEmptyString, "XML Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_OK)
        {
            std::wstring file_path = saveFileDialog.GetPath().ToStdWstring();
            SaveXML(file_path);
            current_xml_file = file_path;
        }
    }
}

void MainWindow::SaveXML(const std::wstring& filepath)
{
    pugi::xml_document doc;

    auto node_date_intervals = doc.append_child(L"date_intervals");

    for (size_t index = 0; index < dataStore.GetDateIntervalsSize(); ++index)
    {
        auto node_interval = node_date_intervals.append_child(L"interval");

        auto attribute_begin_date = node_interval.append_attribute(L"begin_date");
        std::string begin_date_iso_string = boost::gregorian::to_iso_string(dataStore.GetDateIntervalConstRef(index).begin());
        attribute_begin_date.set_value(std::wstring(begin_date_iso_string.begin(), begin_date_iso_string.end()).c_str());
        
        auto attribute_end_date = node_interval.append_attribute(L"end_date");
        std::string end_date_iso_string = boost::gregorian::to_iso_string(dataStore.GetDateIntervalConstRef(index).end());
        attribute_end_date.set_value(std::wstring(end_date_iso_string.begin(), end_date_iso_string.end()).c_str());
    }
   
    page_setup_panel->SaveXML(&doc);
    title_setup_panel->SaveToXML(&doc);
    elements_setup_panel->SaveToXML(&doc);
    calendar_setup_panel->SaveToXML(&doc);
    
    doc.save_file(filepath.c_str());
}

void MainWindow::LoadXML(const std::wstring& filepath)
{
    pugi::xml_document doc;

    auto load_success = doc.load_file(filepath.c_str());

    if (load_success)
    {
        auto node_date_intervals = doc.child(L"date_intervals");

        std::vector<date_period> temporary;

        for (auto& node_interval : node_date_intervals.children(L"interval"))
        {
            std::wstring begin_date_string = node_interval.attribute(L"begin_date").value();
            std::wstring end_date_string = node_interval.attribute(L"end_date").value();

            date boost_begin_date = boost::gregorian::from_undelimited_string(std::string(begin_date_string.begin(), begin_date_string.end()));
            date boost_end_date = boost::gregorian::from_undelimited_string(std::string(end_date_string.begin(), end_date_string.end()));

            temporary.push_back(date_period(boost_begin_date, boost_end_date));
        }

        dataStore.SetDateIntervals(temporary);       
    }

    page_setup_panel->LoadXML(doc);
    title_setup_panel->LoadFromXML(doc);
    elements_setup_panel->LoadFromXML(doc);
    calendar_setup_panel->LoadFromXML(doc);
}

void MainWindow::SlotLicenseInfo(wxCommandEvent& event)
{
    LicenseInformationDialog dialog;
    dialog.ShowModal();
}

