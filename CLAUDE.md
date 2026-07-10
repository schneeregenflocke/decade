# CLAUDE.md

Diese Datei enthält Anweisungen für Claude Code (claude.ai/code) und andere Coding-Agents, die in diesem Repository mit dem Code arbeiten. Sie hält die stabilen Leitplanken — Bauen, Architektur, Konventionen, Prinzipien — und am Ende einen Backlog-Abschnitt für offene Punkte.

## Projekt

C++23-Desktopanwendung für Kalender und Zeitachsen. Aus einem Prototyp gewachsen; sie trägt noch technische Schulden (Gott-Klassen, unklare Besitzverhältnisse). Ziel ist evolutionäres Refactoring — schrittweise, verhaltenserhaltend, ohne Rewrite (siehe [Refactoring](#refactoring)).

Abhängigkeiten:

- wxWidgets (https://docs.wxwidgets.org/3.2/) — GUI-Toolkit.
- OpenGL 4.6 core (https://registry.khronos.org/OpenGL-Refpages/gl4/) via libepoxy (https://github.com/anholt/libepoxy) — Rendering.
- ICU (https://unicode-org.github.io/icu/userguide/datetime/calendar/) — Kalenderarithmetik, lokales Datumsparsen und -formatieren, Unicode-Textverarbeitung.
- Boost.Serialization (https://www.boost.org/doc/libs/release/libs/serialization/doc/index.html) — XML-Projektdateien.
- FreeType (https://freetype.org/freetype2/docs/documentation.html) — Text-Rasterung.
- csv2 (https://github.com/p-ranav/csv2) — CSV-Import und -Export.
- Bullet Collision World (https://github.com/bulletphysics/bullet3) — Hit-Testing und Picking.
- sigslot (https://github.com/palacaze/sigslot) — Signal/Slot-Verdrahtung.
- tinycolormap (https://github.com/yuki-koyama/tinycolormap) — Farbskalen.

### Untermodule

Fünf Git-Untermodule liegen unter `external/`: embed-resource (eigener Fork, https://github.com/schneeregenflocke/embed-resource), sigslot (https://github.com/palacaze/sigslot), csv2 (https://github.com/p-ranav/csv2), tinycolormap (https://github.com/yuki-koyama/tinycolormap), bullet3 (https://github.com/bulletphysics/bullet3) — nach dem Klonen mit `git submodule update --init --recursive` initialisieren. Shader und Lizenzen werden über `embed_resources()` in `CMakeLists.txt` ins Binary eingebettet.

## Bauen & Starten

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

### Tests

`tests/` spiegelt die `src/`-Struktur; die wx-freien Header sind direkt unit-getestet. Nach dem Bauen:

```bash
ctest --test-dir build
```

### Startdatei

Eine CSV-Datendatei kann beim Start optional per CLI-Argument importiert werden.

```bash
# CSV-Import
./build/decade examples/sample_dates.csv
```

### Kopflose / skriptgesteuerte Läufe

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

Die Startdatei (CSV oder XML) wird ausschliesslich als Positionsargument übergeben (siehe [Startdatei](#startdatei)); ohne Angabe startet ein leeres Projekt.

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

## Build-Prüfungen & Werkzeuge

### Warnings & clang-tidy-Gate

- **Warnings brechen den Build — behebe sie, unterdrücke sie nicht. Diese Regel gilt sowohl für Compiler-Warnungen als auch für Diagnosen von clang-tidy (https://clang.llvm.org/extra/clang-tidy/).** Kein Finding mit `NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN` (https://clang.llvm.org/extra/clang-tidy/index.html#suppressing-undesired-diagnostics), einem `#pragma`, einem `-Wno-…`-Flag oder einer `.clang-tidy`-Einzelzeilen-Ausnahme einfach ruhigstellen. Ändere den Code so, dass das Finding nicht mehr greift. Umstrukturierung, RAII und korrekte Typannotationen sind Fixes; Unterdrückung ist keiner.
  - **Bevorzuge einen echten Fix, auch wenn die Warnung zunächst "nicht behebbar" wirkt.** Meist ist sie es doch. Beispiel: `cppcoreguidelines-owning-memory` (https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/owning-memory.html) über einer rohen C-Ressource ist erfüllt, wenn Ownership mit `gsl::owner<>` aus der GSL (https://github.com/microsoft/GSL) — das Projekt verlinkt bereits `Microsoft.GSL::GSL` — und/oder einem `std::unique_ptr` mit Custom-Deleter ausgedrückt wird — siehe `src/infrastructure/graphics/png_writer.hpp`. Eine Regel, die für den Projektstil wirklich nicht anwendbar ist, wird **einmal, global** in `.clang-tidy` mit Kommentar deaktiviert (wie bereits `-modernize-use-trailing-return-type`, `-llvm-header-guard`, …) — niemals verteilt pro Zeile.
  - **Unterdrückung ist nur als letzter Ausweg erlaubt, und zwar nur für Konstrukte, die uns ein Third-Party-C-API vertraglich aufzwingt und die im Code nicht anders lösbar sind.** Das kanonische (und derzeit einzige zugelassene) Beispiel ist das zwingende `setjmp`/`longjmp`-Error-Handling von libpng in `src/infrastructure/graphics/png_writer.hpp`. Wenn du unterdrücken musst: scoping des `NOLINT` auf die **konkreten Check-Namen** (niemals ein nacktes `NOLINT`), auf die engste Zeile beschränken und einen Kommentar hinzufügen, der erklärt, *warum* das nicht behebbar ist. Wenn eine ganze Abhängigkeit eine Warnungsklasse unvermeidbar macht, ersetze lieber diese Abhängigkeit, statt Unterdrückungen zu verteilen.

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
- Der Workflow (`runs-on: ubuntu-ci`) läuft in einem `ubuntu:26.04`-Container auf dem **ersten freien Runner mit diesem Label** — das tragen der CI-Runner auf `laptop-omen` (Normalfall) und der homelab-Runner. Landet der Job auf homelab (RPi4), scheitert der Build voraussichtlich am 1200-MB-Limit des dortigen Runners — bewusste Abwägung, damit CI nicht hängt, wenn der Laptop aus ist. Runner-Stack: `docker-stacks/forgejo-runner/docker-compose.ci.yml`, deployt über Komodo (Stack `forgejo-runner-laptop-omen`); Label-Schema im dortigen README.
- Schritte: apt-Abhängigkeiten → Checkout mit Submodulen → `cmake`/`ninja` (mit `-Werror`) → `ctest` → clang-tidy-Gate. Ist der Laptop aus, bleibt der Job in der Queue, bis der Runner wieder pollt.

## Architektur

### Ziele

- Den Startpfad klein und testbar halten.
- UI-Verdrahtung vom App-Bootstrap isolieren.
- Den Datenfluss zwischen Stores, Panels und Renderer explizit halten.
- Klare Schichtung und selbsterklärende Namen, damit der Code navigierbar bleibt — siehe [Selbsterklärender Code](#selbsterklärender-code).

### Schichten & Schichtregeln

Die Codebasis folgt einer Architektur mit vier Schichten im Sinn der Clean Architecture (https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html). Abhängigkeiten fliessen nur **nach innen** (Presentation → Application → Domain; Infrastructure wird von der Application konsumiert). Die Domain kennt nichts anderes; die Infrastructure kennt nur die Domain-Typen, die sie serialisiert. Neuer Code muss angeben, zu welcher Schicht er gehört, und deren Abhängigkeitsgrenzen einhalten.

- **Presentation** — wxWidgets-Panels, GL-Canvas-Wrapper, Menüs, Dialoge. Jedes Panel besitzt seine Widgets und bietet Signale mit derselben Schnittstelle wie sein Store.
- **Application** — Composition Root, EventBus, Binder, Rendering-Adapter, App-Lifecycle: konstruiert, besitzt und verdrahtet, enthält aber selbst keine Domänenlogik.
- **Domain** — Value Objects, Stores, Transformationslogik, sigslot-Signale — UI-agnostisch und Boost-frei (kein `friend boost::serialization::access`, keine `serialize`-Member).
- **Infrastructure** — Rendering (OpenGL, FreeType), Persistenz (XML/CSV/PNG), Hit-Testing (Bullet) — nicht-intrusiv, kennt nur Domain-Typen.

Die Verzeichnisnamen unter `src/` benennen die Schichten direkt: `presentation/`, `application/` (mit dem Rendering-Adapter in `application/calendar/`), `domain/`, `infrastructure/{graphics,physics,persistence}`; dazu `common/` für Querschnitt (z. B. `debug_log.hpp`). `tests/` spiegelt diese Struktur. Welche Datei zu welcher Schicht gehört, zeigt der Baum selbst — der Code ist die Referenz, nicht diese Datei.

Regeln:

1. Der Prozesseinstiegspunkt enthält keine Business- oder UI-Verdrahtungslogik.
2. Der App-Lifecycle konstruiert High-Level-Objekte, kennt aber keine Panel-/Store-Interna.
3. Die Composition Root besitzt alle Lifetimes. Die Verdrahtung lebt getrennt davon im Binder (freie Funktionen) — Komponenten kennen einander nicht direkt.
4. Die Domain bleibt UI-agnostisch — kein wx, kein GL, kein Boost.
5. Stores publizieren ihren Zustand über den EventBus; Konsumenten abonnieren über den Bus statt über einzelne Store-Signale.
6. Infrastructure nimmt Domain-Typen per Referenz; sie darf nicht von Presentation abhängen.
7. Die Kommandozeile wird an genau einer Stelle gelesen und in ein Options-Objekt übersetzt (`src/application/runtime_options.hpp`); Umgebungsvariablen liest die Anwendung nicht.
8. Die Serialisierung ist nicht-intrusiv: Sie arbeitet nur über öffentliche APIs und besitzt das On-Disk-Format; Domain-Typen wissen nichts von Persistenz.
9. Genau eine Brücke verbindet Application und Rendering-Infrastructure: der Rendering-Adapter mit seinem Scene-Composer. Presentation hängt nie von GL-Typen ab — sie sieht den Scene-Graph nur als GL-freies Read-Model (Snapshot).
10. Der anwendungsweite `LocaleDateFormatter` wird einmal in der Composition Root konstruiert und per Referenz weitergereicht — die Locale-Konfiguration bleibt an einer Stelle.

### Domain-Muster: Value Objects & Stores

- **Value Objects (https://martinfowler.com/bliki/ValueObject.html)** halten Daten plus die Abfragen darauf und kapseln ihren Zustand: Datenmember `private`, Zugriff über konstante Accessors, Änderung nur über benannte Setter — so bleiben Invarianten und On-Disk-Format an einer Stelle. Kein Signal, keine Serialisierung, kein `friend` → Rule of Zero (https://en.cppreference.com/w/cpp/language/rule_of_three), frei kopierbar.
- **Stores** kombinieren ein Value Object mit einem `sigslot::signal` und einer Re-Entry-Guard. Sie haben Identität → explizit nicht kopierbar. Das Signal trägt den Value (`signal<const Value&>`), sodass Konsumenten mit kopierbaren Value Objects arbeiten; der Store bietet `Receive`/`Send`/`Get`, keine eigene Query-Delegation.
- Value Object und Store liegen in **separaten Dateien**, benannt nach der Hauptklasse (`date_group.hpp` + `date_group_store.hpp`); der Value-Object-Header hat keine Store-Abhängigkeit.

### Datums- & Intervallsemantik

- `Date` (proleptischer Gregorianischer Kalender, expliziter Invalid-State) und `DatePeriod` sind die datenbankfreie Schnittstelle. Die Kalenderberechnungen sind an ICU delegiert und bewusst auf genau zwei Stellen beschränkt: das interne Arithmetik-Backend und den `LocaleDateFormatter` (sprach- und locale-abhängiges Parsen und Formatieren für GUI und CSV). Ein Wechsel der Datumslibrary heisst: genau diese zwei Stellen neu implementieren — sonst greift niemand direkt auf ICU-Datum-APIs zu.
- Persistiert wird als ISO-8601-String (https://de.wikipedia.org/wiki/ISO_8601); das frühere Boost.DateTime-basierte Projektdateiformat ist **nicht mehr lesbar** (bewusster Formatbruch).
- `DatePeriod` ist **überall im Modell einheitlich halb-offen `begin, end)`** (zur Begründung: [Dijkstra, EWD831 (https://www.cs.utexas.edu/users/EWD/transcriptions/EWD08xx/EWD831.html)) — `LengthDays()` ist `end - begin`, `Last()` der Tag vor `end`, und ein Zeitraum ohne enthaltenen Tag (`end <= begin`) ist *null* (die Stores verwerfen null Periods). Nutzer denken in *inklusiven* "von .. bis"-Daten; die Umrechnung passiert an genau zwei nutzerseitigen Grenzen — Datumstabellen-Panel und CSV-I/O — via `PeriodFromInclusiveDates()` beim Eingang und `Last()` bei der Anzeige. Nirgends sonst soll ±1-Tag-Arithmetik auftauchen; es soll so bleiben.

### Ereignisfluss (EventBus via sigslot)

Die komponentenübergreifende Kommunikation läuft über einen in-process **EventBus**. Der Bus besitzt pro Domain-Ereignis ein typisiertes `sigslot::signal`. Panels und Stores behalten ihre eigenen `Send*`- / `Receive*`-Methoden, aber die Verdrahtung ist zentral in `main_window_binder::Bind` — jeder Produzent wird in den Bus weitergeleitet, jeder Konsument vom Bus abonniert. `MainWindow::EstablishConnections` ist nur noch ein dünner Wrapper, der `main_window_binder::Bind(...)` gefolgt von `SendInitialValues(...)` aufruft.

```
Panel ──Send──▶ EventBus.<event> ──▶ Store.Receive
Store ──Send──▶ EventBus.<event> ──▶ Panel.Receive , CalendarPage.Receive , GLCanvas.Receive
```

Der `TransformDateEntry`-Adapter sitzt zwischen `date_entries` (Input) und `transformed_date_entries` (Output) auf dem Bus.

Zwei weitere Themen laufen auf demselben Bus. Bei jedem Rebuild emittiert `CalendarPage` ein `SceneNodeSnapshot` (`scene_snapshot`-Topic), das das Scene-Tree-Panel rendert. Für Picking meldet der GL-Canvas Pointer-Bewegungen im Seitenraum an den `InteractionController`, der sie über `CalendarPage::Pick` (ein Bullet-Raycast über die `PickBox`es der Bars) hit-tested und den geänderten Hover über das `hovered`-Topic publiziert; `CalendarPage::ReceiveHovered` färbt die Bar dann direkt um.

```
GLCanvas ──pointer move──▶ InteractionController ──Pick──▶ CalendarPage/PhysicsWorld
InteractionController ──Send──▶ EventBus.hovered ──▶ CalendarPage.ReceiveHovered
CalendarPage ──Send──▶ EventBus.scene_snapshot ──▶ SceneTreePanel.Receive
```

Wenn du einen neuen Zustand hinzufügst: Erzeuge ein Boost-freies **Value Object** plus einen `*Store` darum in `domain/`, ein Panel in `presentation/`, füge ein typisiertes Signal zu `EventBus` hinzu und registriere beide Enden in `main_window_binder::Bind`. Wenn der Zustand persistiert wird, ergänze nicht-intrusive `save`/`load` (oder `serialize`) für den Value in `infrastructure/persistence/value_serialization.hpp` und eine Zeile in `project_io`. `CalendarPage` braucht nur dann einen `Receive*`-Slot, wenn sich der Zustand auf das Rendering auswirkt.

### OpenGL-Initialisierung

Verzögert: `MainWindow` ruft `GLCanvas::InitOpenGL(version, callback)` auf. Der Callback läuft **nachdem** der GL-Context aktuell ist — dort wird `CalendarPage` konstruiert und `main_window_binder::Bind` ausgeführt. Vor diesem Callback darf kein GL-Zustand angefasst werden.

## Konventionen

- C++23, keine Compiler-Erweiterungen.
- Header-Guards benutzen den Dateinamenstil: der grossgeschriebene Dateiname mit dem Punkt vor dem Suffix als `_`, z. B. `main_window.hpp` → `MAIN_WINDOW_HPP`, `gl_canvas.hpp` → `GL_CANVAS_HPP`. Kein Verzeichnispfadpräfix. Das konsequent in `#ifndef`, `#define` und dem abschliessenden `#endif  // <GUARD>`-Kommentar anwenden. (Die clang-tidy-Prüfung `llvm-header-guard`, die sonst einen Full-Path-Stil erzwingen würde, ist in `.clang-tidy` deaktiviert — so lassen.)

### Selbsterklärender Code

Der Code kommuniziert seine Absicht selbst; Prosa ist die Ausnahme. Leitplanke ist P.1 «Express ideas directly in code» der C++ Core Guidelines (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rp-direct).

- **Namen tragen den Zweck, nicht den Mechanismus** — auf allen Ebenen: Variablen, Funktionen, Klassen, Member. Anker: Intention-Revealing Selector (Kent Beck, «Smalltalk Best Practice Patterns») für Namen; Intention-Revealing Interfaces (Eric Evans, DDD Reference, https://www.domainlanguage.com/ddd/reference/) für Schnittstellen; General Naming Rules (https://google.github.io/styleguide/cppguide.html#General_Naming_Rules): für Lesbarkeit optimieren, keine kryptischen Abkürzungen.
- **Struktur erklärt sich selbst:** kleine Einheiten mit einer Verantwortung; eine Abstraktionsebene pro Funktion (Single Level of Abstraction Principle, SLAP); tiefe Module — kleine Schnittstelle, viel Funktionalität dahinter (Deep Modules; John Ousterhout, «A Philosophy of Software Design», https://web.stanford.edu/~ouster/cgi-bin/book.php).
- **Kommentare sind sparsam** und erklären nur das nicht-offensichtliche Warum (Entscheidung, Trade-off), nie das Was (NL.1 der Core Guidelines, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-naming). Ein Kommentar, der beschreibt, *was* der Code tut, ist ein Hinweis, den Code klarer zu machen — nicht, den Kommentar zu behalten.

### Naming & Style

Konvention: Google C++ Style (https://google.github.io/styleguide/cppguide.html#Naming) (in Kraft):

- Typen: `PascalCase` (`DateGroup`).
- Funktionen & Methoden: `PascalCase` (`GetDateGroups()`); triviale Accessor/Mutator dürfen `snake_case` wie ihr Member heissen (`set_count()`).
- Klassen-Datenmember: `snake_case` **mit Trailing-Underscore** (`date_format_`). Struct-Member ohne Underscore. Diese Member-Regel wird vom clang-tidy-Gate erzwungen (`readability-identifier-naming` (https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html) in `.clang-tidy`); ein Member ohne Underscore bricht den Build.
- Locals: `snake_case`. Konstanten/Enumeratoren: `kPascalCase` (`kColorScale`).
- Der Store-Suffix ist einheitlich `…Store` (kein `…Storage`) — bei Typen **und** bei Member-/Parameternamen (`…_store`, nicht `…_storage`).
- Renames zur Vereinheitlichung von Schreibweise und Bezeichnern sind erwünscht. Wenn umbenannt wird, dann **vollständig und konsistent** über alle Vorkommen (Deklaration, Definition, Aufrufstellen, Tests, Doku) — kein halber Rename, der zwei Schreibweisen nebeneinander stehen lässt. Den Build danach grün halten (Compile + `ctest` + clang-tidy-Gate).

### Ownership & Lifetimes

- Niemals rohes `new`/`delete` verwenden. Ownership immer über Smart Pointer ausdrücken, damit die Lifetime im Typsystem kodiert ist, Ausnahmen keine Ressourcen leaken und Ownership-Übergaben an Call-Sites explizit sind (vgl. C++ Core Guidelines, Resource management (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-resource)). wx-Widgets werden typischerweise via `.release()` an wx-owned Parents übergeben (wx besitzt dann die Lifetime).
- Reihenfolge der Wahl: Wertsemantik / Stack / Container zuerst → `std::unique_ptr` für exklusiven Besitz → `std::shared_ptr` *nur*, wenn Besitz nachweislich geteilt ist (ein Design-Signal, kein Default) → `std::weak_ptr` gegen Zyklen.
- Rohzeiger sind nie Besitzer (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr); als nicht-besitzende Beobachter sind sie nur in Parametern und Locals zulässig. **Raw-Pointer-Datenmember sind verboten** — auch nicht-ownende: ein rohes `T*`-Member kodiert weder Ownership noch Lifetime und ist die klassische Dangling-Pointer-Falle. Drücke die Beziehung stattdessen im Typsystem aus:
  - **Nicht-ownende Referenz auf ein von `wxTrackable` abgeleitetes Objekt** (wx-owned Parents/Children — `wxWindow`, `wxEvtHandler` und die meisten wx-Klassen): `wxWeakRef<T>` (https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html) verwenden. Es setzt sich automatisch auf null zurück, wenn das referenzierte Objekt zerstört wird (siehe wxTrackable (https://docs.wxwidgets.org/3.2/classwx_trackable.html)).
  - **Nicht-ownende Referenz auf ein wx-owned Objekt, das *nicht* `wxTrackable` ist** (z. B. `wxPGProperty` erbt von `wxObject`, daher kompiliert `wxWeakRef` nicht): **gar keinen Pointer cachen**. Das Objekt bei Bedarf vom Owner holen — aus dem Event, das es trägt (`wxPropertyGridEvent::GetProperty()`), oder per Namenssuche am Owner (`wxPropertyGrid::GetPropertyByName()`) — mit einer stabilen Namenskonstante, die Erzeugung und Lookup gemeinsam verwenden. Der temporäre `T*`, den eine wx-API am Call-Site zurückgibt, ist in Ordnung; das Speichern als Member nicht.
  - **Ein Heap-Objekt besitzen**: ein Smart Pointer (`std::unique_ptr` / `std::shared_ptr`) oder `MakeOwned<T>`, wenn ein wx-Widget an seinen wx-owned Parent übergeben wird.
- Rule of Five (https://en.cppreference.com/w/cpp/language/rule_of_three): Klassen mit expliziten Destruktoren sollten Copy/Move ebenfalls löschen oder defaulten (siehe `MainWindow`, `DateEntryStore`).

### C++-Disziplin

- const-correctness (https://isocpp.org/wiki/faq/const-correctness) konsequent: SSOT für Veränderlichkeit — was nicht verändert wird, ist `const`.
- Include-Disziplin: Include-Dickicht auflösen (Vorwärtsdeklarationen, minimale Includes, include-what-you-use (https://include-what-you-use.org/)). Grösserer struktureller Hebel als Reformatierung, senkt nebenbei Compile-Zeiten.

### Als Header-only konzipiert

Die Codebasis ist **absichtlich als Header-only konzipiert** (`main.cpp` ist die einzige Translation Unit). Beim Hinzufügen von Code lieber bestehende Header direkt erweitern als in `.cpp` aufzuteilen. Diese Konvention auch bei Refactorings beibehalten. Definitionen, die in einem Header leben, müssen `inline` (https://en.cppreference.com/w/cpp/language/inline) sein (freie Funktionen und out-of-class Member-Definitionen), damit die Single-TU-Regel ODR-Verstösse nicht stillschweigend verdeckt, falls ein Header irgendwann von woanders eingebunden wird (z. B. Tests).

### Stil (Sprache & Doku)

- Kommentare und Doku: knappes, aktives Deutsch (Wolf Schneider (https://de.wikipedia.org/wiki/Wolf_Schneider)). So wenige Zeilen wie möglich; nur Nicht-Offensichtliches kommentieren.
- Echte Umlaute schreiben (ä/ö/ü), nie ae/oe/ue. Schweizer Rechtschreibung: ss statt ß.
- In Markdown-Dateien keine Tabellen verwenden; stattdessen mehrzeilige Listen. Sie sind leichter lesbar, leichter zu schreiben und oft kürzer.

## Entwurfsprinzipien

Das sind die verbindlichen Entwurfsprinzipien für diese Codebasis. Die etablierten Fachbegriffe sind hier — wie im ganzen Dokument — bewusst als semantische Anker gesetzt (Semantic Anchors, https://github.com/LLM-Coding/Semantic-Anchors): Der Begriff aktiviert das dahinterliegende Wissen, bei Menschen wie bei Coding-Agents, präziser als jede Umschreibung.

- Single Responsibility Principle (https://en.wikipedia.org/wiki/Single-responsibility_principle) und Separation of Concerns (https://en.wikipedia.org/wiki/Separation_of_concerns); geringe Kopplung (https://en.wikipedia.org/wiki/Coupling_%28computer_programming%29), hohe Kohäsion (https://en.wikipedia.org/wiki/Cohesion_%28computer_science%29).
- Domain-Driven Design (https://www.domainlanguage.com/ddd/reference/) — siehe das [Domain-Muster](#domain-muster-value-objects--stores) — und Clean Architecture (https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) — siehe [Schichten & Schichtregeln](#schichten--schichtregeln).
- DRY (https://en.wikipedia.org/wiki/Don%27t_repeat_yourself) als Wissens-DRY, massgeblich ist Single Source of Truth (https://en.wikipedia.org/wiki/Single_source_of_truth): jedes Stück Wissen (Regel, Konstante, Domänenentscheidung) hat genau eine autoritative Repräsentation — nicht jede ähnlich aussehende Zeile zusammenfalten. Aber: Duplikation ist billiger als die falsche Abstraktion (https://sandimetz.com/blog/2016/1/20/the-wrong-abstraction); zwei zufällig identische Blöcke, die *verschiedene* Konzepte ausdrücken, bleiben getrennt — im Zweifel nicht verfrüht abstrahieren (YAGNI, https://en.wikipedia.org/wiki/You_aren%27t_gonna_need_it; KISS, https://en.wikipedia.org/wiki/KISS_principle).
  - Mechanik: wenn dieselbe mehrzeilige Form über mehrere Methoden (oder Panels) wiederkehrt, ziehe sie in einen kleinen Helfer hoch — ein `private`-Member, eine freie Funktion oder eine gemeinsame Basisklasse — statt sie zu kopieren. Etablierte Beispiele: `scene_shapes::FillRectangles` / `AddCenteredText` (Scene-Node-Erzeugung), `GLCanvas::ReadBackBuffer` (Rendern + `glReadPixels` + Zeilenflip), `MakeOwned<T>` (parent-owned Widgets), `TablePanelBase` (Tabellen-plus-Add/Delete-Gerüst), `serialization_detail::ColorToArray` / `ColorFromArray` (glm::vec4-Marshalling). Bevorzuge dies gegenüber Makros, weil Makros Lesbarkeit und Debuggability verschlechtern — die expliziten, feldweisen `save`/`load`-Paare in `infrastructure/persistence/value_serialization.hpp` bleiben absichtlich ausgeschrieben, weil sie das On-Disk-Format dokumentieren.
- Selbsterklärender Code — Namen, Struktur und sparsame Kommentare tragen die Absicht: siehe [Selbsterklärender Code](#selbsterklärender-code).
- Principle of Least Astonishment (https://en.wikipedia.org/wiki/Principle_of_least_astonishment): Namen, Signaturen und Verhalten passen zusammen.
- Die kleinste nützliche Abstraktion wählen; expliziten Datenfluss vor versteckter Kopplung bevorzugen (Law of Demeter, https://en.wikipedia.org/wiki/Law_of_Demeter); unübersichtliche Konstrukte einkapseln statt sie zu verbreiten.
- Stabile Regeln von instabilem Backlog trennen (diese Datei vs. der [Backlog-Abschnitt](#arbeitsnotizen--backlog)).

### GRASP-Muster

Wende die GRASP (https://en.wikipedia.org/wiki/GRASP_%28object-oriented_design%29)-Heuristiken zur Verantwortungszuweisung an, wenn du entscheidest, wo Code hingehört:

- Information Expert
- Creator
- Controller
- Low Coupling / High Cohesion
- Indirection
- Pure Fabrication
- Polymorphism / Protected Variations

## Refactoring

- Sicherheit vor Struktur. Vor einer strukturellen Änderung an einer God Class oder einem anderen Code Smell (https://en.wikipedia.org/wiki/Code_smell) zuerst eine Black-Box-Schutzschicht ergänzen: einen Charakterisierungstest (https://en.wikipedia.org/wiki/Characterization_test) mit Eingabe und dem aktuellen Output als eingefrorener Erwartung. Kein Umbau ohne diese Absicherung.
- Verhalten beim Aufräumen nicht ändern. Formatting, Umbenennungen, Warnungsbereinigung und Verhaltensänderungen sind getrennte Schritte oder Commits. Beim Reparieren einer Warnung nie stillschweigend die Semantik ändern.
- Kleine, umkehrbare Schritte. Ein Commit, ein Ziel. Diffs klein genug halten, um sie sauber zurückdrehen zu können. Für grössere Umbauten: Mikado-Methode (Ellnestam/Brolund) — Ziel notieren, Voraussetzungen explorieren, bei Bruch zurückrollen statt durchdrücken.
- Refactoring ist evolutionär, kein Rewrite. Erst das Risiko senken, dann entlang der tragenden Abstraktion schneiden.
- Reine Formatierungscommits isolieren und in `.git-blame-ignore-revs` eintragen (`git blame --ignore-revs-file` (https://git-scm.com/docs/git-blame)), damit die Historie lesbar bleibt.
- Schrittweiser Ablauf: zuerst stabilisieren (Charakterisierungstests, Smoke-Pfade, Baseline-Output) → dann aufteilen (kleine Nähte extrahieren — Seams nach Michael Feathers, «Working Effectively with Legacy Code» —, Verhalten unverändert) → danach umbenennen (Absicht sichtbar machen, ohne Semantik zu ändern) → zuletzt entkoppeln (Kopplung erst entfernen, wenn die Form bereits sicher ist).
- Wenn etwas nicht sofort geändert werden kann, im [Backlog-Abschnitt](#arbeitsnotizen--backlog) notieren, damit es nicht verloren geht.
- Beim Arbeiten mit einer Datei aufgefallene Verstösse gegen die Konventionen dieser Datei werden — auch wenn sie nicht Teil der Aufgabe sind — als TODO im [Backlog](#arbeitsnotizen--backlog) notiert.

## Arbeitsnotizen & Backlog

Diese Datei hält die stabilen Leitplanken; offene Punkte, Fragen und Historik leben in diesem Abschnitt (die frühere `TODO.txt` ist hier aufgegangen). Neue Gedanken kurz halten und direkt neben die Regel setzen, die sie betreffen; Refactoring-Notizen bleiben hier, bis eine Entscheidung stabil genug für ein eigenes Dokument ist.

### Offene TODOs

- **wxWidgets-3.4-Upgrade, sobald in Arch `extra` (abgeklärt 2026-07-10):** Jetzt **nicht** upgraden — abwarten. Die 3.3-Serie bleibt offiziell der Development-Zweig; die erste stabile Serie mit den 3.3-Features wird **3.4.0** (https://wxwidgets.org/blog/2025/12/planning-ahead/). Arch `extra` führt weiterhin nur 3.2.11; 3.3 gibt es nur im AUR (`wxwidgets-gtk3-unstable`, `-git`). Beim Upgrade dann:
  - Prüfen, ob das Paket mit GLX **und** EGL gebaut ist — die Laufzeitwahl (seit 3.3.2, `wxGLCanvas::PreferGLX()` bzw. Env `wx_opengl_egl=0`, https://github.com/wxWidgets/wxWidgets/pull/26023) nützt nur dann. Das heutige Arch-3.2.11 ist EGL-only gelinkt (kein GLX) — deshalb bricht `GDK_BACKEND=x11` die EGL-Surface. Mit GLX-Fallback die Screenshot-Anleitung unter [Kopflose / skriptgesteuerte Läufe](#kopflose--skriptgesteuerte-läufe) neu prüfen (Xvfb evtl. verzichtbar).
  - Relevante inkompatible Änderungen der 3.3-Linie (docs/changes.txt, *INCOMPATIBLE CHANGES*): `wxGLCanvas` nutzt kein Multi-Sampling mehr per Default; `CreateSurface()` entfällt bei EGL-Builds; `wxBitmap::Create(size, dc)` skaliert nicht mehr mit dem Content-Scale-Faktor; `wxImageList`-Grössen in physischen Pixeln. Upstream stuft 3.3 als «almost fully compatible» mit 3.2 ein.
  - Doku-Links von /3.2/ auf die neue Serie nachziehen.
  - Optionaler Zwischenschritt, falls das GLX/EGL-Feature vorab getestet werden soll: wx 3.3.3 lokal nach `~/opt` bauen (beide GL-Backends aktivieren) und decade via `CMAKE_PREFIX_PATH` dagegen bauen — Systempakete bleiben unangetastet; `find_package(wxWidgets CONFIG)` ist versionsagnostisch.
- **Repo ordnen und aufräumen (notiert 2026-07-10):** Struktur und Inhalte des ganzen Repositorys ordnen, nach demselben Vorgehen wie beim CLAUDE.md-Aufräumen (siehe Erledigt): Sichten → Trennen → Versorgen → Verdichten. Teilstück `src/`-/`tests/`-Struktur ist erledigt (siehe «`src/` auf Clean-Architecture-Verzeichnisse umgestellt»); offen bleibt der Rest des Repos (Root-Dateien, `examples/`, `shaders/`, Doku).
- **`CalendarPage` von `GLCanvas` entkoppeln (notiert 2026-07-10):** `application/calendar/calendar_page.hpp` inkludiert `presentation/gl_canvas.hpp` — der Rendering-Adapter hängt damit an einem Presentation-Panel, entgegen der Abhängigkeitsrichtung (Regeln 6/9). Die benötigte Fähigkeit (Redraw/Context) über ein schmales Interface oder einen Callback hereinreichen.
- **`calendar_section_builders.hpp` aufteilen (notiert 2026-07-10):** Mit 582 Zeilen die grösste Datei im Baum; entlang der Section-Builder in kleinere Header splitten (vorher Charakterisierungstest, siehe [Refactoring](#refactoring)).

### Offene Folgefragen

- Welche 2 bis 3 Hotspots sollen zuerst charakterisiert werden?
- Wie strikt soll Header-only als harte Vorgabe bleiben?
- Wie viel API-Härtung mit non-null/span soll jetzt passieren?
- Wie viel Include-Umschreiben soll pro Sprint automatisiert werden?
- Wie weit soll die EventBus-Zentralisierung gehen, bevor ein pragmatischer Hybrid bleibt?

### Weiterführende Refactoring-Ziele

1. Stores sollen direkt in den `EventBus` publizieren, sobald alle Konsumenten bus-only sind.
2. Das aktuelle Verhalten soll erhalten bleiben, während die Anzahl der Stellen sinkt, die von einem bestimmten Zustand wissen müssen.

### Erledigt (Historik)

- ~~CI via Forgejo-Runner~~ (2026-07-10) — Build-/Test-Workflow auf https://git.blem.ch/ eingerichtet und end-to-end verifiziert; Beschreibung der Kette im Abschnitt [CI](#ci). Die Abklärung ergab: ja, Forgejo ordnet Jobs per Runner-Label zu — der CI-Runner läuft als Komodo-deployter Container auf `laptop-omen` (host-neutraler Stack `docker-stacks/forgejo-runner/docker-compose.ci.yml`). Label-Schema seither: `runner-<host>` exklusiv pro Runner plus geteiltes `ubuntu-ci` für CI-Jobs. Nebenbefund behoben: Ubuntu liefert keine wxWidgets-CMake-Config → Module-Mode-Fallback in `CMakeLists.txt`.
- ~~`src/` auf Clean-Architecture-Verzeichnisse umgestellt~~ (2026-07-10) — die Ordnernamen benennen jetzt die Schichten 1:1: `packages/` → `domain/` (Namespace `packages::detail` → `domain::detail`), `gui/` → `presentation/`, `graphics/`+`physics/` → `infrastructure/{graphics,physics}`, `app/services/` → `infrastructure/persistence/` (Namespaces → `persistence::…`), `app/` → `application/`; `app/binding/` aufgelöst in Verdrahtung (`application/event_bus.hpp`, `main_window_binder.hpp`) und Rendering-Adapter (`application/calendar/`). Dazu: `scene_snapshot.hpp` in die Domain (rein std-basiert; löste den Presentation→Application-Include), `debug_log.hpp` nach `common/` (Querschnitt), Renames `opengl_panel.hpp` → `gl_canvas.hpp` und `graphics/icu.hpp` → `unicode.hpp`. Testspiegel und CMake nachgezogen; verhaltenserhaltend in kleinen Commits (Build + `ctest` + clang-tidy-Gate grün).
- ~~wxWidgets 3.2 → 3.3 abklären~~ (2026-07-10) — Ergebnis: warten auf 3.4 in Arch `extra`; Details und Upgrade-Checkliste im TODO «wxWidgets-3.4-Upgrade» oben. Kern: 3.3 ist Development-Zweig, Arch paketiert ihn nicht offiziell, und die GLX/EGL-Laufzeitwahl braucht ein mit beiden Backends gebautes Paket (das heutige Arch-Paket ist EGL-only).
- ~~Kopflose Läufe auf CLI-Argumente umstellen~~ (2026-07-10) — die `DECADE_*`-Umgebungsvariablen ersatzlos durch GNU-Long-Options ersetzt (`--dump-png`, `--exit-after-ms`, …, `--debug-log`; siehe [Kopflose / skriptgesteuerte Läufe](#kopflose--skriptgesteuerte-läufe)). Definition und Parsen des Vokabulars an genau einer Stelle (`AddRuntimeOptions` / `RuntimeOptionsFromParser` in `src/application/runtime_options.hpp`, via `wxCmdLineParser`); `DECADE_DEBUG_LOG` läuft jetzt über `decade_debug::SetLogEnabled` statt `getenv`; `DECADE_DEFAULT_CSV` gestrichen (Startdatei nur noch als Positionsargument).
- ~~CLAUDE.md ordnen und straffen~~ (2026-07-10) — dedupliziert (einkopierter `TODO.txt`-Block aufgelöst, Prinzipien-Listen vereint), nach Nutzungsfluss neu gegliedert, Referenz-Links ergänzt, Untermodul-Liste korrigiert (fünf statt drei).
- ~~Style-Migration auf Google C++ Style~~ — Klassen-Datenmember tragen den Trailing-Underscore; vom Gate erzwungen via `readability-identifier-naming` (siehe [Naming & Style](#naming--style)).
- ~~Datei- und Naming-Struktur in `packages/` (heute `domain/`)~~ — Value-Object und Store je eigene Datei (nach Hauptklasse benannt), Store-Suffix einheitlich `…Store`; auch die Member-/Parameternamen sind auf `…_store` vereinheitlicht (kein `…_storage`).
- ~~Unit-Tests für die CSV-/XML-Konvertierung~~ — `tests/infrastructure/persistence/test_csv_io.cpp` und `tests/infrastructure/persistence/test_value_serialization.cpp` decken Lesen/Schreiben, Rundläufe und Grenzfälle ab; die CSV-/XML-Logik liegt dafür in eigenen, wx-freien Headern (`infrastructure/persistence/csv_io.hpp`, `infrastructure/persistence/value_serialization.hpp`).
- ~~`PageSetupConfig` als gekapseltes Value-Object~~ — war als einziges Value-Object ein öffentliches Aggregat; jetzt private Member mit `Size()`/`Margins()`/`Orientation()`-Accessoren und `Set*`-Settern, genau wie die übrigen Value-Objects. Persistenz läuft non-intrusiv über die `save`/`load`-Paarung in `infrastructure/persistence/value_serialization.hpp`.
