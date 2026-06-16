#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include <wx/string.h>
#include <wx/utils.h>

#include <cstddef>
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
//   DECADE_DUMP_PNG_DPI=<dpi>     export DPI for DECADE_DUMP_PNG; defaults to
//                                 GLCanvas::kExportPngDpi when unset.
//   DECADE_DUMP_WINDOW_PNG=<path> capture the window back buffer after paint.
//   DECADE_DUMP_FRAME_PNG=<path>  capture the whole main frame (tabs + panels +
//                                 canvas) to PNG via wxDC, compositing the GL
//                                 back buffer on top. The widget read-back
//                                 needs the X11 backend (a wxClientDC blit
//                                 returns black under Wayland), so run it
//                                 headless under Xvfb (see CLAUDE.md, "Headless
//                                 / scripted runs", for the full xvfb-run
//                                 line).
//   DECADE_SELECT_TAB=<label>     pre-select a notebook tab by its label
//                                 (case-insensitive) at startup, e.g. for
//                                 screenshotting a specific tab.
//   DECADE_EXIT_AFTER_MS=<ms>     auto-close the main window after N ms.
//   DECADE_DEBUG_HOVER_BAR=<n>    force the hover highlight on bar N after load
//                                 (debug/screenshot aid for picking, no mouse).
//   DECADE_DEBUG_SELECT_NODE=<p>  force the scene-tree selection highlight on
//                                 the node at path `p` ("root/.../name") after
//                                 load (debug/screenshot aid, no mouse).
//
// (DECADE_DEBUG_LOG is read by the Infrastructure layer in
// `src/graphics/debug_log.hpp`, which must not depend on this Application
// header, so it is intentionally not mirrored here.)
struct RuntimeOptions {
  // File to load at startup. Opt-in: empty means "start with an empty project".
  std::optional<std::string> startup_file;
  std::optional<std::string> dump_png_path;
  // Export DPI for dump_png_path; falls back to GLCanvas::kExportPngDpi.
  std::optional<int> dump_png_dpi;
  std::optional<std::string> dump_window_png_path;
  std::optional<std::string> dump_frame_png_path;
  std::optional<std::string> select_tab;
  std::optional<std::int64_t> exit_after_ms;
  // Debug/screenshot aid: force the hover highlight on this bar index at
  // startup, so the picking highlight can be verified without a pointer device.
  std::optional<std::size_t> debug_hover_bar;
  // Debug/screenshot aid: force the scene-tree selection highlight on this node
  // path at startup, so the selection overlay can be verified without a mouse.
  std::optional<std::string> debug_select_node;
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

  if (const std::optional<std::string> dump_png_dpi =
          GetEnvString("DECADE_DUMP_PNG_DPI")) {
    try {
      const int parsed = std::stoi(*dump_png_dpi);
      if (parsed > 0) {
        options.dump_png_dpi = parsed;
      }
    } catch (const std::exception& ex) {
      std::cerr << "Invalid DECADE_DUMP_PNG_DPI: " << ex.what() << '\n';
    }
  }
  options.dump_frame_png_path = GetEnvString("DECADE_DUMP_FRAME_PNG");
  options.select_tab = GetEnvString("DECADE_SELECT_TAB");

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

  if (const std::optional<std::string> debug_hover_bar =
          GetEnvString("DECADE_DEBUG_HOVER_BAR")) {
    try {
      const long long parsed = std::stoll(*debug_hover_bar);
      if (parsed >= 0) {
        options.debug_hover_bar = static_cast<std::size_t>(parsed);
      }
    } catch (const std::exception& ex) {
      std::cerr << "Invalid DECADE_DEBUG_HOVER_BAR: " << ex.what() << '\n';
    }
  }

  if (std::optional<std::string> debug_select_node =
          GetEnvString("DECADE_DEBUG_SELECT_NODE")) {
    options.debug_select_node = std::move(*debug_select_node);
  }

  return options;
}

}  // namespace app

#endif  // RUNTIME_OPTIONS_HPP
