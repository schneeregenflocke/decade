#ifndef STARTUP_SCRIPT_HPP
#define STARTUP_SCRIPT_HPP

#include <wx/filefn.h>

#include <iostream>
#include <string>

#include "../infrastructure/graphics/pick_id.hpp"
#include "../presentation/gl_canvas.hpp"
#include "../presentation/main_frame.hpp"
#include "calendar/calendar_page.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"

namespace application {

// Setzt die Kommandozeile in Handlungen um — Startdatei laden, Tab vorwählen,
// Bild schreiben, nach N Millisekunden schliessen. Vorher lagen diese Schritte
// als sechs Methoden quer über das Hauptfenster verstreut; hier stehen sie
// beisammen, und das Fenster weiss nichts mehr von Laufzeitoptionen.
//
// Zwei Zeitpunkte, weil OpenGL verzögert bereitsteht: was ohne Kontext geht,
// läuft sofort, der Rest erst danach.
class StartupScript {
 public:
  StartupScript(const RuntimeOptions& options, ProjectDocument& document)
      : options_(options), document_(document) {}

  void RunBeforeGraphics(MainFrame& frame) const {
    SelectStartupTab(frame);
    if (options_.exit_after_ms) {
      std::cout << "Auto-exit in ms: " << *options_.exit_after_ms << '\n';
      frame.CloseAfter(*options_.exit_after_ms);
    }
  }

  void RunAfterGraphics(MainFrame& frame, CalendarPage& calendar_page) const {
    LoadStartupFile();
    ApplyDebugHighlights(frame, calendar_page);
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

  // Opt-in: geladen wird nur, was als Positionsargument kam. Ohne Angabe
  // startet ein leeres Projekt; einen Vorgabepfad relativ zum
  // Arbeitsverzeichnis gibt es bewusst nicht.
  void LoadStartupFile() const {
    if (!options_.startup_file) {
      return;
    }
    const std::string& path = *options_.startup_file;

    if (!wxFileExists(path)) {
      std::cout << "LoadStartupFile: " << path << " not found, skipping\n";
      return;
    }

    std::cout << "LoadStartupFile: loading " << path << '\n';
    if (path.ends_with(".xml")) {
      // Kopflose Läufe: Fehler auf die Konsole statt in einen modalen Dialog,
      // der einen --exit-after-ms-Lauf blockieren würde.
      if (const auto error = document_.LoadXml(path)) {
        std::cout << "LoadStartupFile: " << *error << '\n';
      }
      return;
    }
    document_.ImportCsv(path);
  }

  void ApplyDebugHighlights(MainFrame& frame,
                            CalendarPage& calendar_page) const {
    // Gehovert ist immer höchstens ein Element, darum schliessen sich die
    // beiden Hover-Optionen aus.
    if (options_.debug_hover_title) {
      calendar_page.ReceiveHovered(
          PickId{.kind = PickId::Kind::kTitle, .index = 0});
    } else if (options_.debug_hover_bar) {
      calendar_page.ReceiveHovered(PickId{.kind = PickId::Kind::kBar,
                                          .index = *options_.debug_hover_bar});
    }
    if (options_.debug_select_node) {
      // Über den echten Selektionspfad (Baum -> Detailgrid -> Bus ->
      // Highlight), damit dies das Panel genau wie ein Klick durchläuft.
      frame.SceneTree().SelectNodeByPath(*options_.debug_select_node);
    }
  }

  void WriteRequestedImages(MainFrame& frame) const {
    if (options_.dump_png_path) {
      const int dpi = options_.dump_png_dpi.value_or(GLCanvas::kExportPngDpi);
      std::cout << "--dump-png: writing " << *options_.dump_png_path << " at "
                << dpi << " dpi\n";
      frame.Canvas().SavePNG(*options_.dump_png_path, dpi);
    }
    if (options_.dump_frame_png_path) {
      const std::string path = *options_.dump_frame_png_path;
      // Erst nach dem ersten echten Paint, damit jedes Panel gezeichnet ist.
      frame.CallAfter([&frame, path]() {
        std::cout << "--dump-frame-png: writing " << path << '\n';
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
