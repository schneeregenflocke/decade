/*
Decade
Copyright (c) 2019-2022 Marco Peyer

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

#include "main_window.hpp"

#include <locale>
#include <string>

#ifdef _WIN32
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

class App : public wxApp {
public:
  App() : main_window(nullptr) {}

  virtual bool OnInit()
  {
    wx_locale = std::make_unique<wxLocale>();
    auto init_locale_succeeded = wx_locale->Init();
    std::locale::global(std::locale(""));

    // std::cout << "init_locale_succeeded " << init_locale_succeeded << '\n';
    //  auto language = wxLocale::GetSystemLanguage();
    // std::cout << "current locale name " <<
    // wx_locale->GetLanguageName(language) << '\n';

    const std::string application_name = "Decade";

    constexpr int kMainWindowPosX = 100;
    constexpr int kMainWindowPosY = 100;
    constexpr int kMainWindowWidth = 1280;
    constexpr int kMainWindowHeight = 800;

    main_window = std::make_unique<MainWindow>(
        application_name, wxPoint(kMainWindowPosX, kMainWindowPosY),
        wxSize(kMainWindowWidth, kMainWindowHeight));

    return true;
  }

private:
  std::unique_ptr<MainWindow> main_window;
  std::unique_ptr<wxLocale> wx_locale;
};

wxIMPLEMENT_APP(App);

// wxIMPLEMENT_APP_NO_MAIN(App);
/*int main(int argc, char* argv[])
{
    wxEntryStart(argc, argv);

    wxTheApp->CallOnInit();
    wxTheApp->OnRun();
    wxTheApp->OnExit();

    wxEntryCleanup();

    return EXIT_SUCCESS;
}*/
