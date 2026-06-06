#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

#include <wx/app.h>
#include <wx/intl.h>

#include <locale>
#include <memory>

#include "app_config.hpp"
#include "main_window.hpp"

class DecadeApp : public wxApp {
 public:
  bool OnInit() override {
    locale_.Init();
    std::locale::global(std::locale(""));

    const app::MainWindowConfig window_config = app::DefaultMainWindowConfig();
    auto* main_window =
        std::make_unique<MainWindow>(nullptr, window_config.title,
                                     window_config.position, window_config.size,
                                     window_config.maximize_on_start)
            .release();
    SetTopWindow(main_window);
    return true;
  }

 private:
  wxLocale locale_;
};

#endif  // DECADE_APP_HPP
