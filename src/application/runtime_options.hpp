#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include <wx/cmdline.h>
#include <wx/string.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

namespace application {

// The runtime options for non-interactive runs (CI, screenshots, smoke tests)
// plus the startup file. `AddRuntimeOptions` below defines the vocabulary
// including the --help text; what an option does stands there.
//
// Two things the parser does not show: `--dump-frame-png` needs the X11 backend,
// because a wxClientDC blit delivers black under Wayland (hence under Xvfb, see
// operations.md, "Headless runs"); `--debug-hover-title` beats
// `--debug-hover-bar`, because only one element is hovered at a time.
struct RuntimeOptions {
  // The file to load at start. Opt-in: empty means "an empty project".
  std::optional<std::string> startup_file;
  std::optional<std::string> dump_png_path;
  // The export DPI for dump_png_path; the fallback is GLCanvas::kExportPngDpi.
  std::optional<int> dump_png_dpi;
  std::optional<std::string> dump_frame_png_path;
  std::optional<std::string> select_tab;
  std::optional<std::int64_t> exit_after_ms;
  // A debug and screenshot aid: it forces the hover highlight onto this bar
  // index at start, so the picking highlight is checkable without a pointing
  // device.
  std::optional<std::size_t> debug_hover_bar;
  // The same for the title, which as the only element of its kind needs no
  // index.
  bool debug_hover_title{false};
  // A debug and screenshot aid: it opens the title edit and types the value.
  std::optional<std::string> debug_edit_title;
  // A debug and screenshot aid: it forces the selection highlight onto this
  // node path at start, so the selection overlay is checkable without a mouse.
  std::optional<std::string> debug_select_node;
  bool debug_log{false};
};

// The mark of a non-interactive run: an image capture or an auto exit is asked
// for. No modal dialogue may stand there — it blocks the run until the timeout
// instead of reporting. This run needs a display too; headless in the literal
// sense it is not.
[[nodiscard]] inline bool IsNonInteractiveRun(const RuntimeOptions& options) {
  return options.dump_png_path.has_value() ||
         options.dump_frame_png_path.has_value() ||
         options.exit_after_ms.has_value();
}

// Registers every runtime option at the parser. The descriptions turn up in the
// --help output.
inline void AddRuntimeOptions(wxCmdLineParser& parser) {
  parser.AddLongOption("dump-png",
                       "render the calendar page to PNG (off-screen FBO)");
  parser.AddLongOption("dump-png-dpi", "export DPI for --dump-png",
                       wxCMD_LINE_VAL_NUMBER);
  parser.AddLongOption("dump-frame-png",
                       "capture the whole main frame to PNG (needs X11/Xvfb)");
  parser.AddLongOption("select-tab",
                       "pre-select a notebook tab by label (case-insensitive)");
  parser.AddLongOption("exit-after-ms", "auto-close the main window after N ms",
                       wxCMD_LINE_VAL_NUMBER);
  parser.AddLongOption("debug-hover-bar", "force the hover highlight on bar N",
                       wxCMD_LINE_VAL_NUMBER);
  parser.AddLongSwitch("debug-hover-title",
                       "force the hover highlight on the title");
  parser.AddLongOption("debug-edit-title",
                       "open the title editor and type the given text");
  parser.AddLongOption("debug-select-node",
                       "force the scene-tree selection on node path "
                       "root/.../name");
  parser.AddLongSwitch("debug-log",
                       "enable debug logging; route wx asserts to stderr");
  parser.AddParam("file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
}

namespace runtime_options_detail {
inline std::optional<std::string> FoundString(const wxCmdLineParser& parser,
                                              const wxString& name) {
  wxString value;
  if (parser.Found(name, &value)) {
    return value.ToStdString();
  }
  return std::nullopt;
}
}  // namespace runtime_options_detail

// Builds the options out of the parsed command line parser. Implausible numeric
// values get ignored with a warning on stderr.
inline RuntimeOptions RuntimeOptionsFromParser(const wxCmdLineParser& parser) {
  using runtime_options_detail::FoundString;

  RuntimeOptions options;
  if (parser.GetParamCount() > 0) {
    options.startup_file = parser.GetParam(0).ToStdString();
  }
  options.dump_png_path = FoundString(parser, "dump-png");
  options.dump_frame_png_path = FoundString(parser, "dump-frame-png");
  options.select_tab = FoundString(parser, "select-tab");
  options.debug_select_node = FoundString(parser, "debug-select-node");
  options.debug_hover_title = parser.Found("debug-hover-title");
  options.debug_edit_title = FoundString(parser, "debug-edit-title");
  options.debug_log = parser.Found("debug-log");

  if (long dump_png_dpi = 0; parser.Found("dump-png-dpi", &dump_png_dpi)) {
    if (dump_png_dpi > 0) {
      options.dump_png_dpi = static_cast<int>(dump_png_dpi);
    } else {
      std::cerr << "--dump-png-dpi must be positive; ignored\n";
    }
  }

  if (long exit_after_ms = 0; parser.Found("exit-after-ms", &exit_after_ms)) {
    if (exit_after_ms > 0) {
      options.exit_after_ms = exit_after_ms;
    } else {
      std::cerr << "--exit-after-ms must be positive; ignored\n";
    }
  }

  if (long debug_hover_bar = 0;
      parser.Found("debug-hover-bar", &debug_hover_bar)) {
    if (debug_hover_bar >= 0) {
      options.debug_hover_bar = static_cast<std::size_t>(debug_hover_bar);
    } else {
      std::cerr << "--debug-hover-bar must be >= 0; ignored\n";
    }
  }

  return options;
}

}  // namespace application

#endif  // RUNTIME_OPTIONS_HPP
