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

#include <locale>
#include <memory>
#include <string>

#include <wx/app.h>
#include <wx/gdicmn.h>
#include <wx/intl.h>

#ifdef __WXGTK__
#include <wx/gtk/app.h>
#elif defined(__WXMSW__)
#include <wx/msw/app.h>
#elif defined(__WXOSX__)
#include <wx/osx/app.h>
#endif

#include "main_window.hpp"

class App : public wxApp {
public:
  bool OnInit() override
  {
    wx_locale.Init();
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

    auto *main_window =
        std::make_unique<MainWindow>(nullptr, application_name,
                                     wxPoint(mainWindowPosX, mainWindowPosY),
                                     wxSize(mainWindowWidth, mainWindowHeight))
            .release();
    SetTopWindow(main_window);

    return true;
  }

private:
  wxLocale wx_locale;
};

wxIMPLEMENT_APP(App);
