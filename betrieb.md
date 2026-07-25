# decade — Build & Betrieb

Build, Tests, kopflose Läufe und die Lint-Gates. Prinzipien, Architektur und Konventionen stehen in [AGENTS.md](AGENTS.md).

## Build

Gebaut wird mit CMake (https://cmake.org/cmake/help/latest/) und dem Ninja-Generator (https://ninja-build.org/manual.html). Das Build-Verzeichnis ist `build/`; `compile_commands.json` wird für clangd/clang-tidy exportiert.

Neu konfigurieren (nur wenn sich `CMakeLists.txt` oder Abhängigkeiten ändern):

```bash
cmake -S . -B build -G Ninja
```

Bauen (aus dem Repository-Root; die Ninja-Datei liegt in `build/`):

```bash
ninja -C build
```

GUI starten:

```bash
./build/decade
```

## Tests

`tests/` spiegelt die `src/`-Struktur; die wx-freien Header sind direkt unit-getestet. Nach dem Bauen:

```bash
ctest --test-dir build
```

## Startdatei

Eine Startdatei (CSV oder XML) wird ausschliesslich als Positionsargument übergeben; ohne Angabe startet ein leeres Projekt.

```bash
# CSV-Import
./build/decade examples/sample_dates.csv
```

## Kopflose Läufe

Das Binary beachtet mehrere Kommandozeilen-Optionen in GNU-Syntax (`--name=wert` oder `--name wert`; `--help` zeigt alle) für nicht-interaktive Nutzung — CI, Screenshots, Smoke-Tests. Das Options-Vokabular ist an genau einer Stelle definiert und geparst (`AddRuntimeOptions` / `RuntimeOptionsFromParser` in `src/application/runtime_options.hpp`); Umgebungsvariablen liest das Binary keine mehr.

**Bildaufnahme** — drei Optionen erfassen drei unterschiedliche Dinge; sie sind nicht redundant:

- `--dump-png=<path>` — nur das Kalender-**Seitenbild** über ein off-screen FBO. Auflösung: Export-DPI, weisser Hintergrund, keine App-Chrome. Benötigt: nur OpenGL.
- `--dump-window-png=<path>` — das **GL-Canvas-Pane** exakt wie auf dem Bildschirm (`glReadPixels` auf dem Back Buffer). Auflösung: Bildschirmauflösung, dunkle Ränder um die Seite. Benötigt: nur OpenGL, funktioniert unter Wayland.
- `--dump-frame-png=<path>` — das **gesamte Fenster**: Tabs + Panels (`wxClientDC`-Blit) mit dem oben auf das GL-Back-Buffer komponierten Inhalt. Auflösung: Bildschirmauflösung. Benötigt: Widget-Read-back nur mit X11/Xvfb, unter Wayland leer.

`--dump-window-png` ist die Canvas-only-Untermenge von `--dump-frame-png`; verwende es, wenn du nur das gerenderte Canvas brauchst (und Xvfb vermeiden willst), und `--dump-png`, wenn du einen sauberen High-DPI-Export der Seite selbst brauchst. Alle Dumps werden via `CallAfter` verzögert, damit der erste Paint bereits stattgefunden hat.

**Steuerung:**

- `--dump-png-dpi=<dpi>` — Export-DPI für `--dump-png`; standardmässig `GLCanvas::kExportPngDpi` (200), wenn nicht gesetzt. Wird z. B. für hochaufgelöste README-Renderings verwendet.
- `--exit-after-ms=<ms>` — schliesst das Hauptfenster nach N ms automatisch.
- `--select-tab=<label>` — wählt beim Start einen Notebook-Tab per Label vor (case-insensitive), z. B. zum Screenshotten eines bestimmten Tabs.
- `--debug-log` — aktiviert OpenGL-/Runtime-Debug-Logging. Leitet ausserdem wx-**Assert-Fehler nach stderr weiter und läuft weiter** statt einen modalen Dialog zu öffnen, damit headless/screenshotte Läufe einen fehlschlagenden `wxASSERT` sichtbar machen (statt still zu blockieren) — siehe `DecadeApp::OnAssertFailure`.
- `--debug-hover-bar=<index>` — hebt beim Start die Bar mit dem angegebenen Index hervor, als wäre sie gehovert, um den Hover-Pfad ohne Live-Cursor zu screenshotten oder zu debuggen.
- `--debug-select-node=<path>` — wählt beim Start den Scene-Tree-Node an `path` (`root/.../name`) aus und durchläuft damit den realen Selektionspfad (Scene-Tab-Detailgrid + Kalender-Selection-Highlight des Knotens und seines Teilbaums) ohne Zeigegerät.

Typischer Smoke-Test (Sample-Daten explizit mitgeben):

```bash
stdbuf -oL -eL timeout 12 ./build/decade \
  --dump-png=/tmp/decade_render.png --exit-after-ms=2000 examples/sample_dates.csv
```

Vollständiger UI-Screenshot (Tabs + Panels + Canvas) eines bestimmten Tabs. Der Widget-Read-back funktioniert nur auf dem **X11-Backend** — ein `wxClientDC`-Blit liefert unter Wayland schwarz — also headless unter **Xvfb** mit Software-GL ausführen. Das ist der unterstützte Weg, die echte GUI zu screenshotten: GNOME/Wayland blockiert programmatisches Screen-Capture, und `GDK_BACKEND=x11` auf einer laufenden XWayland-Session bricht die EGL-Surface des GL-Canvas.

```bash
xvfb-run -a -s "-screen 0 1600x1000x24" \
  env GDK_BACKEND=x11 LIBGL_ALWAYS_SOFTWARE=1 \
  timeout 30 ./build/decade --select-tab=Timeframe \
  --dump-frame-png=/tmp/decade_ui.png --exit-after-ms=3000 examples/sample_dates.csv
```

## Build-Prüfungen

Die Regel dahinter — **Warnings brechen den Build, nie unterdrücken** — steht in [AGENTS.md](AGENTS.md), Abschnitt Konventionen. Hier die Befehle.

**Durchsetzungs-Gate (Build-Brecher).** Das Gate führt clang-tidy auf der einzelnen Translation Unit `src/application/main.cpp` aus — die transitiv alle `src/`-Header via `HeaderFilterRegex` abdeckt — und stuft jedes Finding als Error ein:

```bash
cmake --build build --target clang-tidy   # schlägt bei JEDEM Finding fehl
```

Der Baum wird bei **null Findings** gehalten; ein neues Finding bricht dieses Target. Das Gate ist bewusst *nicht* Teil des Standard-Builds, damit normale Compiles schnell bleiben — explizit oder in CI ausführen. Bevorzuge diese Single-TU-Form gegenüber direktem Globbing von `src/**/*.hpp`: Header isoliert zu analysieren erzeugt künstliches `misc-include-cleaner`-Rauschen für die GL-/wx-Umbrella-Header, das in einer echten Translation Unit nie auftritt. Die in `.clang-tidy` deaktivierten Checks tragen jeweils einen Kommentar, der erklärt, warum (glm-Unions, GL-/wx-C-API-Interop, absichtlicher Stil).

Voller Lauf (Bericht in `build/clang-tidy.log`):

```bash
clang-tidy -p build \
  --extra-arg=-include --extra-arg=type_traits \
  --extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option \
  src/**/*.cpp src/**/*.hpp 2>&1 | tee build/clang-tidy.log
```

Auto-Fix für eine einzelne Check-Gruppe (Beispiel `modernize-*`, ohne `--fix-errors`):

```bash
clang-tidy -p build \
  --extra-arg=-include --extra-arg=type_traits \
  --extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option \
  --fix --fix-notes \
  --checks='-*,modernize-*,-modernize-use-trailing-return-type' \
  src/**/*.cpp src/**/*.hpp
```

Warum die Extra-Args:

- `-include type_traits` umgeht einen Bug in `wx/meta/convertible.h`, das `std::is_base_of` ohne `<type_traits>`-Include verwendet. GCC zieht das transitiv ein, Clang nicht.
- `-Wno-error` und `-Wno-unknown-warning-option` verhindern, dass GCC-spezifische Flags aus `compile_commands.json` (`-Wlogical-op`, `-Wduplicated-branches`, ...) zusammen mit `-Werror` aus dem Build zu Compiler-Errors in clang-tidy werden.

`src/**/*.cpp src/**/*.hpp` setzt voraus, dass die Shell rekursive Globs unterstützt (zsh standardmässig; bash erst nach `shopt -s globstar`).

### clang-format

`.clang-format` ist die verbindliche Quelle für das Formatting (ClangFormat-Doku (https://clang.llvm.org/docs/ClangFormat.html)).

```bash
find . -regex '.*\.\(cpp\|cxx\|hpp\|cc\|h\)' -not -path './build/*' -not -path './external/*' -exec clang-format -style=file -i {} +
```

### CI

CI läuft auf der eigenen Forgejo-Instanz (https://git.blem.ch/), nicht auf GitHub Actions (die alte Workflow-Datei liegt deaktiviert unter `.github/workflows/cmake.yml.disable`). Die Kette, analog zu `blem-website`:

- `git push origin main` (GitHub) → GitHub-Webhook ruft die `mirror-sync`-API → Pull-Mirror `github-mirror/decade` synct sofort → der Sync feuert das push-Event für `.forgejo/workflows/build.yml`.
- Der Workflow (`runs-on: runner-laptop-omen`) läuft in einem `ubuntu:26.04`-Container **exklusiv** auf dem CI-Runner auf `laptop-omen` — `runner-<host>` ist das exklusive Runner-Label (vgl. geteiltes `ubuntu-ci`). Der schwere C++-Build (Boost/wx/Submodule) darf nicht auf homelab (RPi4 + USB-SMR) landen: dort scheitert er am 1200-MB-Limit und treibt die Platte in I/O-Sättigung (loadavg-Vorfall 2026-07-11). Ist der Laptop aus, wartet der Job in der Queue, statt auf homelab zu fallen. Runner-Stack: `docker-stacks/forgejo-runner/docker-compose.ci.yml`, deployt über Komodo (Stack `forgejo-runner-laptop-omen`); Label-Schema im dortigen README.
- Schritte: apt-Abhängigkeiten → Checkout mit Submodulen → `cmake`/`ninja` (mit `-Werror`) → `ctest` → clang-tidy-Gate. Ist der Laptop aus, bleibt der Job in der Queue, bis der Runner wieder pollt.
