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

    // _CRTDBG_MAP_ALLOC
    //_CrtDumpMemoryLeaks(); ??

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

    date_groups_table_panel = new DateGroupsTablePanel(book_panel);
    page_setup_panel = new PageSetupPanel(book_panel);
    data_table_panel = new DateTablePanel(book_panel);
    font_setup_panel = new FontSetupPanel(book_panel);
    title_setup_panel = new TitleSetupPanel(book_panel);
    elements_setup_panel = new ElementsSetupsPanel(book_panel);
    calendar_setup_panel = new CalendarSetupPanel(book_panel);

    list_box->AppendString(date_groups_table_panel->GetPanelName());
    list_box->AppendString(L"Date Table");
    list_box->AppendString(L"Page Setup");
    list_box->AppendString(L"Font Setup");
    list_box->AppendString(L"Title Setup");
    list_box->AppendString(L"Calendar Setup");
    list_box->AppendString(L"Elements Setup");
    
    book_panel_sizer = new wxBoxSizer(wxVERTICAL);
    book_panel->SetSizer(book_panel_sizer);

    wxSizerFlags sizer_flags;
    sizer_flags.Proportion(1).Expand().Border(wxALL, 5);

    book_panel_sizer->Add(date_groups_table_panel, sizer_flags);
    book_panel_sizer->Add(data_table_panel, sizer_flags);
    book_panel_sizer->Add(page_setup_panel, sizer_flags);
    book_panel_sizer->Add(font_setup_panel, sizer_flags);
    book_panel_sizer->Add(title_setup_panel, sizer_flags);
    book_panel_sizer->Add(calendar_setup_panel, sizer_flags);
    book_panel_sizer->Add(elements_setup_panel, sizer_flags);
    

    book_panel_sizer->ShowItems(false);

    Bind(wxEVT_LISTBOX, &MainWindow::SlotSelectListBook, this);

    const size_t default_list_box_selection = 0;
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

    date_groups_table_panel->signal_table_date_groups.connect(&DateGroupStore::SetDateGroups, &date_groups_store);
    
    date_groups_store.signal_date_groups.connect(&DateGroupsTablePanel::UpdateTable, date_groups_table_panel);
    date_groups_store.signal_date_groups.connect(&DateTablePanel::UpdateGroups, data_table_panel);
    date_groups_store.signal_date_groups.connect(&ElementsSetupsPanel::UpdateGroups, elements_setup_panel);
    date_groups_store.InitDefault();


    date_interval_bundle_store.signal_date_interval_bundles.connect(&DateTablePanel::UpdateTable, data_table_panel);
    data_table_panel->signal_table_date_interval_bundles.connect(&DateIntervalBundleStore::SetDateIntervalBundles, &date_interval_bundle_store);

    transformed_date_interval_bundle.SetTransform(0, 1);
    date_interval_bundle_store.signal_date_interval_bundles.connect(&TransformDateIntervalBundle::InputDateIntervals, &transformed_date_interval_bundle);
    transformed_date_interval_bundle.signal_transformed_date_interval_bundles.connect(&CalendarPage::SetDateIntervalBundles, calendar.get());


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

    calendar_setup_panel->signal_calendar_config.connect(&CalendarPage::SlotCalendarConfig, calendar.get());
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

        for(size_t index = 0; index < date_interval_bundle_store.GetDateIntervalsSize(); ++index)
        {
            auto begin_date = boost_date_to_string(date_interval_bundle_store.GetDateIntervalConstRef(index).begin());
            auto last_date = boost_date_to_string(date_interval_bundle_store.GetDateIntervalConstRef(index).end());
            
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

    std::vector<DateIntervalBundle> buffer;
   
    while (csv_reader.busy())
    {
        if (csv_reader.ready())
        {
            ////////////////////////////////////////////////////////////////////////////////
            auto row = csv_reader.next_row(); // auto& row = csv_reader.next_row();

            auto begin_date_string = row[std::to_string(0)];
            auto end_date_string = row[std::to_string(1)];
            
            auto begin_date = string_to_boost_date(begin_date_string, date_format);
            auto end_date = string_to_boost_date(end_date_string, date_format);
            
            // check for date_period
            if ( (begin_date.is_special() || end_date.is_special() ) == false)
            {
                date_period date_interval(begin_date, end_date);

                if (date_interval.is_null() == false)
                {
                    buffer.push_back(date_interval);
                }
            }

            // check for single date
            if ( begin_date.is_special() == false && end_date_string.empty() )
            {
                date_period date_interval(begin_date, begin_date);

                buffer.push_back(date_interval);
            }
        }
    }

    date_interval_bundle_store.SetDateIntervalBundles(buffer);
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

    date_groups_store.SaveXML(&doc);
    date_interval_bundle_store.SaveXML(&doc);
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
        date_groups_store.LoadXML(doc);
        date_interval_bundle_store.LoadXML(doc);
        page_setup_panel->LoadXML(doc);
        title_setup_panel->LoadFromXML(doc);
        elements_setup_panel->LoadFromXML(doc);
        calendar_setup_panel->LoadFromXML(doc);
    }
}

void MainWindow::SlotLicenseInfo(wxCommandEvent& event)
{
    LicenseInformationDialog dialog;
    dialog.ShowModal();
}

