#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/string.h>

#include <exception>
#include <iostream>
#include <memory>

#include "../common/debug_log.hpp"
#include "app_composition.hpp"
#include "locale_services.hpp"
#include "runtime_info.hpp"
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

  // A runtime debug aid: with --debug-log, wx assert failures go to stderr and
  // the app keeps running instead of opening a modal dialogue. The dialogue
  // blocks headless and screenshot runs (and CI) — a real assertion would
  // otherwise end silently by timeout instead of being reported. This way the
  // assertion text lands on stderr and the run carries on.
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

    try {
      locale_services_ = std::make_unique<application::LocaleServices>();
    } catch (const std::exception& exception) {
      std::cerr << exception.what() << '\n';
      return false;
    }

    application::PrintRuntimeInfo(std::cout);

    composition_ = std::make_unique<application::AppComposition>(
        locale_services_->date_formatter(), runtime_options_);
    SetTopWindow(&composition_->Frame());
    return true;
  }

 private:
  std::unique_ptr<application::LocaleServices> locale_services_;
  std::unique_ptr<application::AppComposition> composition_;
  application::RuntimeOptions runtime_options_;
};

#endif  // DECADE_APP_HPP
