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

// #include <wx/setup.h>
#include <wx/wx.h>

#include "main_window.hpp"
#include <gsl/gsl>
#include <locale>
#include <memory>
#include <string>

class App : public wxApp {
public:
  bool OnInit() override
  {
    wx_locale = std::make_unique<wxLocale>();
    auto init_locale_succeeded = wx_locale->Init();
    std::locale::global(std::locale(""));
    // std::cout << "init_locale_succeeded " << init_locale_succeeded << '\n';
    // auto language = wxLocale::GetSystemLanguage();
    // std::cout << "current locale name " <<
    // wx_locale->GetLanguageName(language) << '\n';

    const std::string application_name = "Decade";

    constexpr int mainWindowPosX = 100;
    constexpr int mainWindowPosY = 100;
    constexpr int mainWindowWidth = 1280;
    constexpr int mainWindowHeight = 800;

    gsl::owner<MainWindow *> main_window = nullptr;
    main_window = new MainWindow(nullptr, application_name, wxPoint(mainWindowPosX, mainWindowPosY),
                                 wxSize(mainWindowWidth, mainWindowHeight));
    SetTopWindow(main_window);

    return true;
  }

private:
  std::unique_ptr<wxLocale> wx_locale;
};

wxIMPLEMENT_APP(App);
