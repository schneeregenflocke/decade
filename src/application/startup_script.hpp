#ifndef STARTUP_SCRIPT_HPP
#define STARTUP_SCRIPT_HPP

#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <iostream>
#include <string>

#include "../common/debug_log.hpp"
#include "../infrastructure/graphics/pick_id.hpp"
#include "../presentation/gl_canvas.hpp"
#include "../presentation/main_frame.hpp"
#include "calendar/calendar_page.hpp"
#include "calendar/title_text_editor.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"

namespace application {

// Turns the command line into actions — load the startup file, preselect a tab,
// write an image, close after N milliseconds. These steps used to sit scattered
// across the main window as six methods; here they stand together, and the
// window knows nothing of runtime options any more.
//
// Two moments, because OpenGL stands ready with a delay: whatever works without
// a context runs at once, the rest afterwards.
class StartupScript {
 public:
  StartupScript(const RuntimeOptions& options, ProjectDocument& document)
      : options_(options), document_(document) {}

  void RunBeforeGraphics(MainFrame& frame) const {
    SelectStartupTab(frame);
    if (options_.exit_after_ms) {
      if (decade_debug::LogEnabled()) {
        std::cout << "Auto-exit in ms: " << *options_.exit_after_ms << '\n';
      }
      frame.CloseAfter(*options_.exit_after_ms);
    }
  }

  void RunAfterGraphics(MainFrame& frame, CalendarPage& calendar_page,
                        TitleTextEditor& title_text_editor) const {
    LoadStartupFile();
    ApplyDebugHighlights(frame, calendar_page, title_text_editor);
    WriteRequestedImages(frame);
  }

 private:
  void SelectStartupTab(MainFrame& frame) const {
    if (!options_.select_tab) {
      return;
    }
    if (!frame.SelectTab(*options_.select_tab)) {
      std::cerr << "--select-tab: no tab labelled '" << *options_.select_tab
                << "'\n";
    }
  }

  // Opt-in: what came as a positional argument gets loaded, and nothing else.
  // Without one an empty project starts; a default path relative to the working
  // directory deliberately does not exist.
  void LoadStartupFile() const {
    if (!options_.startup_file) {
      return;
    }
    const std::string& path = *options_.startup_file;

    if (!QFileInfo::exists(QString::fromStdString(path))) {
      std::cerr << "LoadStartupFile: " << path << " not found, skipping\n";
      return;
    }

    if (decade_debug::LogEnabled()) {
      std::cout << "LoadStartupFile: loading " << path << '\n';
    }
    if (path.ends_with(".xml")) {
      // Headless runs: errors to the console instead of into a modal dialogue,
      // which would block an --exit-after-ms run.
      if (const auto error = document_.LoadXml(path)) {
        std::cerr << "LoadStartupFile: " << *error << '\n';
      }
      return;
    }
    document_.ImportCsv(path);
  }

  void ApplyDebugHighlights(MainFrame& frame, CalendarPage& calendar_page,
                            TitleTextEditor& title_text_editor) const {
    // At most one element is ever hovered, so the two hover options exclude
    // each other.
    if (options_.debug_hover_title) {
      calendar_page.ReceiveHovered(
          PickId{.kind = PickId::Kind::kTitle, .index = 0});
    } else if (options_.debug_hover_bar) {
      calendar_page.ReceiveHovered(PickId{.kind = PickId::Kind::kBar,
                                          .index = *options_.debug_hover_bar});
    }
    if (options_.debug_edit_title) {
      title_text_editor.Begin(PickId{.kind = PickId::Kind::kTitle, .index = 0});
      if (!options_.debug_edit_title->empty()) {
        title_text_editor.Insert(*options_.debug_edit_title);
      }
    }
    if (options_.debug_select_node) {
      // Over the real selection path (tree -> detail grid -> bus -> highlight),
      // so this walks the panel exactly as a click would.
      frame.SceneTree().SelectNodeByPath(*options_.debug_select_node);
    }
  }

  void WriteRequestedImages(MainFrame& frame) const {
    if (options_.dump_png_path) {
      const int dpi = options_.dump_png_dpi.value_or(GLCanvas::kExportPngDpi);
      if (decade_debug::LogEnabled()) {
        std::cout << "--dump-png: writing " << *options_.dump_png_path << " at "
                  << dpi << " dpi\n";
      }
      frame.Canvas().SavePNG(*options_.dump_png_path, dpi);
    }
    if (options_.dump_frame_png_path) {
      const std::string path = *options_.dump_frame_png_path;
      // After the first real paint alone, so every panel is drawn: queued on
      // the event loop, which runs it once the pending paints are through.
      QTimer::singleShot(0, &frame, [&frame, path]() {
        if (decade_debug::LogEnabled()) {
          std::cout << "--dump-frame-png: writing " << path << '\n';
        }
        if (!frame.SaveFrameScreenshot(path)) {
          std::cerr << "--dump-frame-png: failed to write " << path << '\n';
        }
      });
    }
  }

  const RuntimeOptions& options_;
  ProjectDocument& document_;
};

}  // namespace application

#endif  // STARTUP_SCRIPT_HPP
