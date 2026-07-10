#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/intl.h>
#include <wx/string.h>

#include <iostream>
#include <locale>
#include <memory>
#include <string>

#include "../common/debug_log.hpp"
#include "app_config.hpp"
#include "main_window.hpp"
#include "runtime_options.hpp"

class DecadeApp : public wxApp {
 public:
  void OnInitCmdLine(wxCmdLineParser& parser) override {
    wxApp::OnInitCmdLine(parser);
    application::AddRuntimeOptions(parser);
  }

  bool OnCmdLineParsed(wxCmdLineParser& parser) override {
    if (!wxApp::OnCmdLineParsed(parser)) {
      return false;
    }
    runtime_options_ = application::RuntimeOptionsFromParser(parser);
    return true;
  }

  // Runtime-Debughilfe: mit --debug-log gehen wx-Assert-Fehler nach stderr
  // und die App läuft weiter, statt einen modalen Dialog zu öffnen. Der Dialog
  // blockiert headless/Screenshot-Läufe (und CI) — eine echte Assertion würde
  // sonst still per Timeout enden statt gemeldet zu werden. So landet der
  // Assertion-Text auf stderr und der Lauf geht weiter.
  void OnAssertFailure(const wxChar* file, int line, const wxChar* func,
                       const wxChar* cond, const wxChar* msg) override {
    if (!runtime_options_.debug_log) {
      wxApp::OnAssertFailure(file, line, func, cond, msg);
      return;
    }
    std::cerr << "wx assert failed: " << wxString(file).ToStdString() << ':'
              << line << " ("
              << (func != nullptr ? wxString(func).ToStdString()
                                  : std::string{})
              << ") " << wxString(cond).ToStdString();
    if (msg != nullptr) {
      std::cerr << " : " << wxString(msg).ToStdString();
    }
    std::cerr << '\n';
  }

  bool OnInit() override {
    // Calling the base OnInit triggers command-line parsing, i.e. our
    // OnInitCmdLine / OnCmdLineParsed overrides above.
    if (!wxApp::OnInit()) {
      return false;
    }

    decade_debug::SetLogEnabled(runtime_options_.debug_log);

    locale_.Init();
    std::locale::global(std::locale(""));

    const application::MainWindowConfig window_config = application::DefaultMainWindowConfig();
    auto* main_window = std::make_unique<MainWindow>(
                            nullptr, window_config.title,
                            window_config.position, window_config.size,
                            window_config.maximize_on_start, runtime_options_)
                            .release();
    SetTopWindow(main_window);
    return true;
  }

 private:
  wxLocale locale_;
  application::RuntimeOptions runtime_options_;
};

#endif  // DECADE_APP_HPP
