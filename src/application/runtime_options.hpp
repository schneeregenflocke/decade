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

// Bündelt die Laufzeit-Optionen, die die Anwendung beim Start liest. Sie
// steuern die nicht-interaktive Nutzung (CI, Screenshots, Smoke-Tests) plus
// die optionale Startdatei. Alle Optionen sind Kommandozeilen-Argumente in
// GNU-Syntax (`--name=wert` oder `--name wert`); `AddRuntimeOptions` und
// `RuntimeOptionsFromParser` unten sind die einzige Stelle, die dieses
// Vokabular definiert und parst.
//
// Erkannte Argumente:
//   <file>                    optionales Positionsargument: Projekt-/Daten-
//                             datei (CSV oder XML), die beim Start geladen
//                             wird; ohne Angabe startet ein leeres Projekt.
//   --dump-png=<path>         rendert die Kalenderseite via Off-Screen-FBO
//                             als PNG.
//   --dump-png-dpi=<dpi>      Export-DPI für --dump-png; ohne Angabe gilt
//                             GLCanvas::kExportPngDpi.
//   --dump-frame-png=<path>   schreibt das gesamte Hauptfenster (Tabs +
//                             Panels + Canvas) via wxDC als PNG, mit dem GL-
//                             Back-Buffer obenauf komponiert. Der Widget-
//                             Read-back braucht das X11-Backend (ein
//                             wxClientDC-Blit liefert unter Wayland schwarz),
//                             also unter Xvfb ausführen (siehe betrieb.md,
//                             «Kopflose Läufe»).
//   --select-tab=<label>      wählt beim Start einen Notebook-Tab per Label
//                             vor (case-insensitive), z. B. zum Screenshotten
//                             eines bestimmten Tabs.
//   --exit-after-ms=<ms>      schliesst das Hauptfenster nach N ms.
//   --debug-hover-bar=<n>     erzwingt nach dem Laden das Hover-Highlight auf
//                             Bar N (Debug-/Screenshot-Hilfe fürs Picking,
//                             ohne Maus).
//   --debug-hover-title       dasselbe für den Titel; schlägt
//                             --debug-hover-bar, da nur ein Element zugleich
//                             gehovert sein kann.
//   --debug-edit-title=<text> öffnet nach dem Laden die Titelbearbeitung wie
//                             ein Doppelklick und tippt <text> hinein (leer:
//                             nur öffnen, mit allem ausgewählt). Die
//                             Bearbeitung bleibt offen, damit Cursor und
//                             Auswahl im Bild stehen.
//   --debug-select-node=<p>   erzwingt nach dem Laden das Selektions-
//                             Highlight des Scene-Tree-Knotens am Pfad `p`
//                             («root/.../name»); Debug-/Screenshot-Hilfe.
//   --debug-log               aktiviert OpenGL-/Runtime-Debug-Logging und
//                             leitet wx-Assert-Fehler nach stderr um (siehe
//                             `DecadeApp::OnAssertFailure`).
struct RuntimeOptions {
  // Beim Start zu ladende Datei. Opt-in: leer heisst «leeres Projekt».
  std::optional<std::string> startup_file;
  std::optional<std::string> dump_png_path;
  // Export-DPI für dump_png_path; Fallback ist GLCanvas::kExportPngDpi.
  std::optional<int> dump_png_dpi;
  std::optional<std::string> dump_frame_png_path;
  std::optional<std::string> select_tab;
  std::optional<std::int64_t> exit_after_ms;
  // Debug-/Screenshot-Hilfe: erzwingt beim Start das Hover-Highlight auf
  // diesem Bar-Index, damit das Picking-Highlight ohne Zeigegerät prüfbar ist.
  std::optional<std::size_t> debug_hover_bar;
  // Dasselbe für den Titel, der als einziges Element seiner Art keinen Index
  // braucht.
  bool debug_hover_title{false};
  // Debug-/Screenshot-Hilfe: öffnet die Titelbearbeitung und tippt den Wert.
  std::optional<std::string> debug_edit_title;
  // Debug-/Screenshot-Hilfe: erzwingt beim Start das Selektions-Highlight auf
  // diesem Knotenpfad, damit das Selektions-Overlay ohne Maus prüfbar ist.
  std::optional<std::string> debug_select_node;
  bool debug_log{false};
};

// Kennzeichen eines nicht-interaktiven Laufs: eine Bildaufnahme oder ein
// Auto-Exit ist angefordert. Dort darf kein modaler Dialog stehen bleiben — er
// blockiert den Lauf bis zum Timeout, statt zu melden. Ein Display braucht auch
// dieser Lauf; headless im Wortsinn ist er nicht.
[[nodiscard]] inline bool IsNonInteractiveRun(const RuntimeOptions& options) {
  return options.dump_png_path.has_value() ||
         options.dump_frame_png_path.has_value() ||
         options.exit_after_ms.has_value();
}

// Registriert alle Laufzeit-Optionen am Parser. Die Beschreibungen erscheinen
// in der --help-Ausgabe.
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

// Baut die Optionen aus dem geparsten Kommandozeilen-Parser. Nicht plausible
// Zahlenwerte werden mit einer stderr-Warnung ignoriert.
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
