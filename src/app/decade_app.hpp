#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/utils.h>

#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <string>

#include "app_config.hpp"
#include "main_window.hpp"
#include "runtime_options.hpp"

class DecadeApp : public wxApp {
 public:
  void OnInitCmdLine(wxCmdLineParser& parser) override {
    wxApp::OnInitCmdLine(parser);
    // Optional positional argument: the project/data file to open at startup
    // (CSV or XML). This is the idiomatic "open with file" entry point and
    // takes precedence over DECADE_DEFAULT_CSV.
    parser.AddParam("file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
  }

  bool OnCmdLineParsed(wxCmdLineParser& parser) override {
    if (!wxApp::OnCmdLineParsed(parser)) {
      return false;
    }
    if (parser.GetParamCount() > 0) {
      cli_startup_file_ = parser.GetParam(0).ToStdString();
    }
    return true;
  }

  // Runtime-debugging aid: when DECADE_DEBUG_LOG=1, route wx assertion failures
  // to stderr and continue instead of popping a modal dialog. The dialog blocks
  // headless/screenshot runs (and CI), so a real assertion would otherwise time
  // out silently rather than being reported. With this, smoke runs surface the
  // assertion text on stderr and keep going.
  void OnAssertFailure(const wxChar* file, int line, const wxChar* func,
                       const wxChar* cond, const wxChar* msg) override {
    if (!log_asserts_to_stderr_) {
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

    wxString debug_log_value;
    log_asserts_to_stderr_ = wxGetEnv("DECADE_DEBUG_LOG", &debug_log_value) &&
                             debug_log_value == "1";

    locale_.Init();
    std::locale::global(std::locale(""));

    app::RuntimeOptions runtime_options = app::RuntimeOptionsFromEnv();
    if (cli_startup_file_) {
      runtime_options.startup_file = cli_startup_file_;  // CLI wins over env
    }

    const app::MainWindowConfig window_config = app::DefaultMainWindowConfig();
    auto* main_window = std::make_unique<MainWindow>(
                            nullptr, window_config.title,
                            window_config.position, window_config.size,
                            window_config.maximize_on_start, runtime_options)
                            .release();
    SetTopWindow(main_window);
    return true;
  }

 private:
  wxLocale locale_;
  std::optional<std::string> cli_startup_file_;
  bool log_asserts_to_stderr_{false};
};

#endif  // DECADE_APP_HPP
