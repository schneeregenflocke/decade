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

#include "gui/wx_widgets_include.h"

#include "main_window.h"

#include <iostream>
#include <string>
#include <locale>


// Reason for use of raw pointers instead of smart_pointers:
// https://wiki.wxwidgets.org/Avoiding_Memory_Leaks 


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
    wxEntryStart(argc, argv);

    wxTheApp->CallOnInit();
    wxTheApp->OnRun();
    wxTheApp->OnExit();
    
    wxEntryCleanup();

    return EXIT_SUCCESS;
}


bool App::OnInit()
{
    std::cout << std::wstring(L"__cplusplus ") + std::to_wstring(__cplusplus) + '\n';

    auto language = wxLocale::GetSystemLanguage();
    locale = std::make_unique<wxLocale>();
    auto init_locale_succeeded = locale->Init(language);
    std::locale::global(std::locale(""));

    std::wcout << L"init_locale_succeeded " << init_locale_succeeded << '\n';
    std::wcout << L"current locale name " << locale->GetLanguageName(language) << '\n';
    
    std::wcout << L"OperatingSystemIdName " << wxPlatformInfo::Get().GetOperatingSystemIdName() << '\n';
    std::wcout << L"ArchName " << wxPlatformInfo::Get().GetArchName() << '\n';

    std::wcout << L"OSMajorVersion.OSMinorVersion.OSMicroVersion " << wxPlatformInfo::Get().GetOSMajorVersion() << '.' <<
        wxPlatformInfo::Get().GetOSMinorVersion() << '.' <<
        wxPlatformInfo::Get().GetOSMicroVersion() << '\n';

    std::wcout << L"wxVERSION_STRING " << wxVERSION_STRING << '\n';
    
    ////////////////////////////////////////////////////////////////////////////////
    
    main_window = new MainWindow("Decade", wxPoint(100, 100), wxSize(1280, 800));

    //main_window->Maximize();
    main_window->Show();
    main_window->Raise();

    std::wcout << "ContentScaleFactor " << main_window->GetContentScaleFactor() << '\n';

    ////////////////////////////////////////////////////////////////////////////////

    std::array<int, 2> gl_version{ 3, 2 };
    main_window->GetGLCanvas()->LoadOpenGL(gl_version);

    return true;
}

