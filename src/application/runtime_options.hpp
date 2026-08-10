#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include <QtCore/QCommandLineParser>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace application {

// The runtime options for non-interactive runs (CI, screenshots, smoke tests)
// plus the startup file. `AddRuntimeOptions` below defines the vocabulary
// including the --help text; what an option does stands there.
//
// One thing the parser does not show: `--debug-hover-title` beats
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
[[nodiscard]] bool IsNonInteractiveRun(const RuntimeOptions& options);

// Registers every runtime option at the parser. The descriptions turn up in the
// --help output.
void AddRuntimeOptions(QCommandLineParser& parser);

// Builds the options out of the parsed command line parser. Implausible numeric
// values get ignored with a warning on stderr.
[[nodiscard]] RuntimeOptions RuntimeOptionsFromParser(
    const QCommandLineParser& parser);

}  // namespace application

#endif  // RUNTIME_OPTIONS_HPP
