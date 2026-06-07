#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include <wx/string.h>
#include <wx/utils.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

namespace app {

// Centralises the runtime knobs the application reads at startup. These drive
// non-interactive use (CI, screenshots, smoke tests) plus the optional startup
// file. Command-line arguments take precedence over the environment (see
// `DecadeApp::OnInit`). Keeping this here means the env-var vocabulary lives in
// one place instead of being scattered across `MainWindow`.
//
// Recognised environment variables:
//   DECADE_DEFAULT_CSV=<path>     opt-in startup file (CSV or XML); nothing is
//                                 loaded unless this (or a CLI argument) is
//                                 set.
//   DECADE_DUMP_PNG=<path>        render the calendar page to PNG via FBO.
//   DECADE_DUMP_WINDOW_PNG=<path> capture the window back buffer after paint.
//   DECADE_EXIT_AFTER_MS=<ms>     auto-close the main window after N ms.
//
// (DECADE_DEBUG_LOG is read by the Infrastructure layer in
// `src/graphics/debug_log.hpp`, which must not depend on this Application
// header, so it is intentionally not mirrored here.)
struct RuntimeOptions {
  // File to load at startup. Opt-in: empty means "start with an empty project".
  std::optional<std::string> startup_file;
  std::optional<std::string> dump_png_path;
  std::optional<std::string> dump_window_png_path;
  std::optional<std::int64_t> exit_after_ms;
};

namespace runtime_options_detail {
inline std::optional<std::string> GetEnvString(const char* name) {
  wxString value;
  if (wxGetEnv(name, &value)) {
    return value.ToStdString();
  }
  return std::nullopt;
}
}  // namespace runtime_options_detail

// Builds the options from the environment. The startup file falls back to
// DECADE_DEFAULT_CSV; callers may override it from the command line afterwards.
inline RuntimeOptions RuntimeOptionsFromEnv() {
  using runtime_options_detail::GetEnvString;

  RuntimeOptions options;
  options.startup_file = GetEnvString("DECADE_DEFAULT_CSV");
  options.dump_png_path = GetEnvString("DECADE_DUMP_PNG");
  options.dump_window_png_path = GetEnvString("DECADE_DUMP_WINDOW_PNG");

  if (const std::optional<std::string> exit_after_ms =
          GetEnvString("DECADE_EXIT_AFTER_MS")) {
    try {
      const std::int64_t parsed = std::stoll(*exit_after_ms);
      if (parsed > 0) {
        options.exit_after_ms = parsed;
      }
    } catch (const std::exception& ex) {
      std::cerr << "Invalid DECADE_EXIT_AFTER_MS: " << ex.what() << '\n';
    }
  }

  return options;
}

}  // namespace app

#endif  // RUNTIME_OPTIONS_HPP
