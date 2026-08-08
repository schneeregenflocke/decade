#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QString>
#include <QtCore/QStringList>
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
[[nodiscard]] inline bool IsNonInteractiveRun(const RuntimeOptions& options) {
  return options.dump_png_path.has_value() ||
         options.dump_frame_png_path.has_value() ||
         options.exit_after_ms.has_value();
}

// Registers every runtime option at the parser. The descriptions turn up in the
// --help output.
inline void AddRuntimeOptions(QCommandLineParser& parser) {
  parser.addOption(
      {"dump-png", "render the calendar page to PNG (off-screen FBO)", "path"});
  parser.addOption({"dump-png-dpi", "export DPI for --dump-png", "dpi"});
  parser.addOption(
      {"dump-frame-png", "capture the whole main frame to PNG", "path"});
  parser.addOption({"select-tab",
                    "pre-select a notebook tab by label (case-insensitive)",
                    "label"});
  parser.addOption(
      {"exit-after-ms", "auto-close the main window after N ms", "ms"});
  parser.addOption(
      {"debug-hover-bar", "force the hover highlight on bar N", "index"});
  parser.addOption(
      {"debug-hover-title", "force the hover highlight on the title"});
  parser.addOption({"debug-edit-title",
                    "open the title editor and type the given text", "text"});
  parser.addOption({"debug-select-node",
                    "force the scene-tree selection on node path root/.../name",
                    "path"});
  parser.addOption({"debug-log", "enable debug logging"});
  parser.addPositionalArgument("file", "project or CSV file to load at start",
                               "[file]");
}

namespace runtime_options_detail {
inline std::optional<std::string> FoundString(const QCommandLineParser& parser,
                                              const QString& name) {
  if (parser.isSet(name)) {
    return parser.value(name).toStdString();
  }
  return std::nullopt;
}

// An option that is set but unreadable as a number counts as absent — with a
// word on stderr, so a typo in a script does not pass unnoticed.
inline std::optional<long long> FoundNumber(const QCommandLineParser& parser,
                                            const QString& name) {
  if (!parser.isSet(name)) {
    return std::nullopt;
  }
  bool parsed = false;
  const long long value = parser.value(name).toLongLong(&parsed);
  if (!parsed) {
    std::cerr << "--" << name.toStdString() << " is not a number; ignored\n";
    return std::nullopt;
  }
  return value;
}
}  // namespace runtime_options_detail

// Builds the options out of the parsed command line parser. Implausible numeric
// values get ignored with a warning on stderr.
inline RuntimeOptions RuntimeOptionsFromParser(
    const QCommandLineParser& parser) {
  using runtime_options_detail::FoundNumber;
  using runtime_options_detail::FoundString;

  RuntimeOptions options;
  const QStringList positional = parser.positionalArguments();
  if (!positional.isEmpty()) {
    options.startup_file = positional.first().toStdString();
  }
  options.dump_png_path = FoundString(parser, "dump-png");
  options.dump_frame_png_path = FoundString(parser, "dump-frame-png");
  options.select_tab = FoundString(parser, "select-tab");
  options.debug_select_node = FoundString(parser, "debug-select-node");
  options.debug_hover_title = parser.isSet("debug-hover-title");
  options.debug_edit_title = FoundString(parser, "debug-edit-title");
  options.debug_log = parser.isSet("debug-log");

  if (const auto dump_png_dpi = FoundNumber(parser, "dump-png-dpi")) {
    if (*dump_png_dpi > 0) {
      options.dump_png_dpi = static_cast<int>(*dump_png_dpi);
    } else {
      std::cerr << "--dump-png-dpi must be positive; ignored\n";
    }
  }

  if (const auto exit_after_ms = FoundNumber(parser, "exit-after-ms")) {
    if (*exit_after_ms > 0) {
      options.exit_after_ms = *exit_after_ms;
    } else {
      std::cerr << "--exit-after-ms must be positive; ignored\n";
    }
  }

  if (const auto debug_hover_bar = FoundNumber(parser, "debug-hover-bar")) {
    if (*debug_hover_bar >= 0) {
      options.debug_hover_bar = static_cast<std::size_t>(*debug_hover_bar);
    } else {
      std::cerr << "--debug-hover-bar must be >= 0; ignored\n";
    }
  }

  return options;
}

}  // namespace application

#endif  // RUNTIME_OPTIONS_HPP
