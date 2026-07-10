# CLAUDE.md

Diese Datei enthält Anweisungen für Claude Code (claude.ai/code) und andere Coding-Agents, die in diesem Repository mit dem Code arbeiten.

## Projekt

C++23-Desktopanwendung für Kalender und Zeitachsen. wxWidgets-GUI, OpenGL (4.6 core) Rendering via libepoxy, ICU für Kalenderarithmetik, lokales Datumsparsen und -formatieren sowie Unicode-Textverarbeitung, Boost.Serialization für XML-Projektdateien, FreeType für Text, csv2 für CSV-Import und -Export, Bullet (Collision World) für Hit-Testing und Picking, sigslot für Signal/Slot-Verdrahtung.

## Stil

- Kommentare und Doku: knappes, aktives Deutsch (Wolf Schneider). So wenige Zeilen wie möglich; nur Nicht-Offensichtliches kommentieren.
- Echte Umlaute schreiben (ä/ö/ü), nie ae/oe/ue. Schweizer Rechtschreibung: ss statt ß.
- In Markdown-Dateien keine Tabellen verwenden; stattdessen mehrzeilige Listen. Sie sind leichter lesbar, leichter zu schreiben und oft kürzer.

## Bauen & Starten

Das CMake-Build-Verzeichnis ist `build/` (Ninja-Generator, `compile_commands.json` für clangd/clang-tidy exportiert).

```bash
# Neu konfigurieren (nur wenn sich CMakeLists.txt oder Abhängigkeiten ändern)
cmake -S . -B build -G Ninja

# Bauen (aus dem Repository-Root; die Ninja-Datei liegt in build/)
ninja -C build

# GUI starten
./build/decade
```

### Startdatei

Eine CSV-Datendatei kann beim Start optional per CLI-Argument importiert werden.

```bash
# CSV-Import
./build/decade test-files/test_dates_1.csv
```

### Kopflose / skriptgesteuerte Läufe

Das Binary beachtet mehrere Umgebungsvariablen (gelesen in `src/app/runtime_options.hpp`) für nicht-interaktive Nutzung — CI, Screenshots, Smoke-Tests.

**Bildaufnahme** — drei Variablen erfassen drei unterschiedliche Dinge; sie sind nicht redundant:

- `DECADE_DUMP_PNG=<path>` — nur das Kalender-**Seitenbild** über ein off-screen FBO. Auflösung: Export-DPI, weisser Hintergrund, keine App-Chrome. Benötigt: nur OpenGL.
- `DECADE_DUMP_WINDOW_PNG=<path>` — das **GL-Canvas-Pane** exakt wie auf dem Bildschirm (`glReadPixels` auf dem Back Buffer). Auflösung: Bildschirmauflösung, dunkle Ränder um die Seite. Benötigt: nur OpenGL, funktioniert unter Wayland.
- `DECADE_DUMP_FRAME_PNG=<path>` — das **gesamte Fenster**: Tabs + Panels (`wxClientDC`-Blit) mit dem oben auf das GL-Back-Buffer komponierten Inhalt. Auflösung: Bildschirmauflösung. Benötigt: Widget-Read-back nur mit X11/Xvfb, unter Wayland leer.

`DUMP_WINDOW_PNG` ist die Canvas-only-Untermenge von `DUMP_FRAME_PNG`; verwende es, wenn du nur das gerenderte Canvas brauchst (und Xvfb vermeiden willst), und `DUMP_PNG`, wenn du einen sauberen High-DPI-Export der Seite selbst brauchst. Alle Dumps werden via `CallAfter` verzögert, damit der erste Paint bereits stattgefunden hat.

**Steuerung:**

- `DECADE_DUMP_PNG_DPI=<dpi>` — Export-DPI für `DECADE_DUMP_PNG`; standardmässig `GLCanvas::kExportPngDpi` (200), wenn unset. Wird z. B. für hochaufgelöste README-Renderings verwendet.
- `DECADE_EXIT_AFTER_MS=<ms>` — schliesst das Hauptfenster nach N ms automatisch.
- `DECADE_SELECT_TAB=<label>` — wählt beim Start einen Notebook-Tab per Label vor (case-insensitive), z. B. zum Screenshotten eines bestimmten Tabs.
- `DECADE_DEBUG_LOG=1` — aktiviert OpenGL-/Runtime-Debug-Logging. Leitet ausserdem wx-**Assert-Fehler nach stderr weiter und läuft weiter** statt einen modalen Dialog zu öffnen, damit headless/screenshotte Läufe einen fehlschlagenden `wxASSERT` sichtbar machen (statt still zu blockieren) — siehe `DecadeApp::OnAssertFailure`.
- `DECADE_DEBUG_HOVER_BAR=<index>` — hebt beim Start die Bar mit dem angegebenen Index hervor, als wäre sie gehovert, um den Hover-Pfad ohne Live-Cursor zu screenshotten oder zu debuggen.
- `DECADE_DEBUG_SELECT_NODE=<path>` — wählt beim Start den Scene-Tree-Node an `path` (`root/.../name`) aus und durchläuft damit den realen Selektionspfad (Scene-Tab-Detailgrid + Kalender-Selection-Highlight des Knotens und seines Teilbaums) ohne Zeigegerät.
- `DECADE_DEFAULT_CSV=<path>` — optionale Startdatei (CSV oder XML), wenn kein Positionsargument gegeben ist; das CLI-Argument hat Vorrang. Es wird keine Datei geladen, wenn dies unset ist.

Typischer Smoke-Test (Sample-Daten explizit mitgeben):
```bash
DECADE_DUMP_PNG=/tmp/decade_render.png DECADE_EXIT_AFTER_MS=2000 \
  stdbuf -oL -eL timeout 12 ./build/decade test-files/test_dates_1.csv
```

Vollständiger UI-Screenshot (Tabs + Panels + Canvas) eines bestimmten Tabs. Der Widget-Read-back funktioniert nur auf dem **X11-Backend** — ein `wxClientDC`-Blit liefert unter Wayland schwarz — also headless unter **Xvfb** mit Software-GL ausführen. Das ist der unterstützte Weg, die echte GUI zu screenshotten: GNOME/Wayland blockiert programmatisches Screen-Capture, und `GDK_BACKEND=x11` auf einer laufenden XWayland-Session bricht die EGL-Surface des GL-Canvas.
```bash
xvfb-run -a -s "-screen 0 1600x1000x24" \
  env GDK_BACKEND=x11 LIBGL_ALWAYS_SOFTWARE=1 \
  DECADE_SELECT_TAB=Timeframe DECADE_DUMP_FRAME_PNG=/tmp/decade_ui.png \
  DECADE_EXIT_AFTER_MS=3000 timeout 30 ./build/decade test-files/test_dates_1.csv
```

## Build-Prüfungen

- **Warnings brechen den Build — behebe sie, unterdrücke sie nicht. Diese Regel gilt sowohl für Compiler-Warnungen als auch für clang-tidy-Diagnosen.** Kein Finding mit `NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN`, einem `#pragma`, einem `-Wno-…`-Flag oder einer `.clang-tidy`-Einzelzeilen-Ausnahme einfach ruhigstellen. Ändere den Code so, dass das Finding nicht mehr greift. Umstrukturierung, RAII und korrekte Typannotationen sind Fixes; Unterdrückung ist keiner.
  - **Bevorzuge einen echten Fix, auch wenn die Warnung zunächst "nicht behebbar" wirkt.** Meist ist sie es doch. Beispiel: `cppcoreguidelines-owning-memory` über einer rohen C-Ressource ist erfüllt, wenn Ownership mit `gsl::owner<>` (das Projekt verlinkt bereits `Microsoft.GSL::GSL`) und/oder einem `std::unique_ptr` mit Custom-Deleter ausgedrückt wird — siehe `src/graphics/png_writer.hpp`. Eine Regel, die für den Projektstil wirklich nicht anwendbar ist, wird **einmal, global** in `.clang-tidy` mit Kommentar deaktiviert (wie bereits `-modernize-use-trailing-return-type`, `-llvm-header-guard`, …) — niemals verteilt pro Zeile.
  - **Unterdrückung ist nur als letzter Ausweg erlaubt, und zwar nur für Konstrukte, die uns ein Third-Party-C-API vertraglich aufzwingt und die im Code nicht anders lösbar sind.** Das kanonische (und derzeit einzige zugelassene) Beispiel ist das zwingende `setjmp`/`longjmp`-Error-Handling von libpng in `src/graphics/png_writer.hpp`. Wenn du unterdrücken musst: scoping des `NOLINT` auf die **konkreten Check-Namen** (niemals ein nacktes `NOLINT`), auf die engste Zeile beschränken und einen Kommentar hinzufügen, der erklärt, *warum* das nicht behebbar ist. Wenn eine ganze Abhängigkeit eine Warnungsklasse unvermeidbar macht, ersetze lieber diese Abhängigkeit, statt Unterdrückungen zu verteilen.
- `.clang-format` ist die verbindliche Quelle für das Formatting.
- Die CI-Workflow-Datei ist derzeit deaktiviert (`.github/workflows/cmake.yml.disable`).

## Refactoring

- Sicherheit vor Struktur. Vor einer strukturellen Änderung an einer God Class oder einem verhedderten Pfad zuerst eine Black-Box-Schutzschicht ergänzen: einen Charakterisierungstest mit Eingabe und dem aktuellen Output als eingefrorener Erwartung.
- Verhalten beim Aufräumen nicht ändern. Formatting, Umbenennungen, Warnungsbereinigung und Verhaltensänderungen sind getrennte Schritte oder Commits.
- Kleine, umkehrbare Schritte. Ein Commit, ein Ziel. Diffs klein genug halten, um sie sauber zurückdrehen zu können.
- Refactoring ist evolutionär, kein Rewrite. Erst das Risiko senken, dann entlang der tragenden Abstraktion schneiden.
- `TODO.txt` bleibt vorläufig bestehen. Dort liegt das laufende Backlog; `CLAUDE.md` soll die Regeln, Grenzen und Arbeitsvereinbarungen kurz und stabil halten.
- Wenn etwas nicht sofort geändert werden kann, in der passenden Sektion notieren, damit es nicht verloren geht.
- Designkriterien, die mitgedacht werden sollen:
  - Intention-revealing names: Namen sollen den Zweck ausdrücken, nicht den Mechanismus.
  - Principle of Least Astonishment: Verhalten soll zur Aussage von Name und Signatur passen.
  - SRP, SoC, geringe Kopplung, hohe Kohäsion.
  - Explizite Ownership und explizite Lifetimes.
  - DRY als Wissens-DRY, nicht als mechanisches Code-Zusammenfalten.
  - Unübersichtliche Konstrukte einkapseln statt sie zu verbreiten.
  - Lieber direkten Ausdruck im Code als Kommentar, wenn der Code es klar sagen kann.
- Offene Folgefragen:
  - Welche 2 bis 3 Hotspots sollen zuerst charakterisiert werden?
  - Wie strikt soll Header-only als harte Vorgabe bleiben?
  - Wie viel API-Härtung mit non-null/span soll jetzt passieren?
  - Wie viel Include-Umschreiben soll pro Sprint automatisiert werden?
  - Wie weit soll die EventBus-Zentralisierung gehen, bevor ein pragmatischer Hybrid bleibt?
- Schrittweiser Ablauf für Refactoring-Arbeiten:
  - Zuerst stabilisieren: Charakterisierungstests, Smoke-Pfade, Baseline-Output.
  - Dann aufteilen: kleine Nähte extrahieren, Verhalten unverändert lassen.
  - Danach umbenennen: Absicht sichtbar machen, ohne Semantik zu ändern.
  - Zuletzt entkoppeln: Kopplung erst entfernen, wenn die Form bereits sicher ist.

## Untermodule

`external/embed-resource`, `external/sigslot`, `external/csv2` sind Git-Untermodule — nach dem Klonen mit `git submodule update --init --recursive` initialisieren. Shader und Lizenzen werden über `embed_resources()` in `CMakeLists.txt` ins Binary eingebettet.

## Als Header-only konzipiert

Die Codebasis ist **absichtlich als Header-only konzipiert** (`main.cpp` ist die einzige Translation Unit). Beim Hinzufügen von Code lieber bestehende Header direkt erweitern als in `.cpp` aufzuteilen. Diese Konvention auch bei Refactorings beibehalten. Definitionen, die in einem Header leben, müssen `inline` sein (freie Funktionen und out-of-class Member-Definitionen), damit die Single-TU-Regel ODR-Verstösse nicht stillschweigend verdeckt, falls ein Header irgendwann von woanders eingebunden wird (z. B. Tests).

## Arbeitsnotizen

- `TODO.txt` als Backlog der offenen Punkte behalten.
- Diese Datei für stabile Leitplanken, Refactoring-Kriterien und Architekturgrenzen nutzen.
- Neue Gedanken kurz halten und direkt neben die Regel setzen, die sie betreffen.
- Markdown in diesem Repository soll keine Tabellen verwenden; stattdessen Listen mit klaren Aufzählungen und kurzen Teilsätzen.
- **TODO — CI via Forgejo-Runner (notiert 2026-07-10):** Für dieses Repo einen
  Build-/Test-Workflow auf https://git.blem.ch/ einrichten, analog zur bestehenden Kette bei
  `github-mirror/blem-website`. Abweichung: Der C++-Build würde den homelab-Server überlasten —
  darum vorher abklären, ob Forgejo den Runner-Job auf den Host `laptop-omen` auslagern kann
  (eigener Runner auf dem Laptop, Job-Zuordnung per Runner-Label). Erst planen, dann umsetzen.
- **TODO — CLAUDE.md aufräumen (notiert 2026-07-10):** Diese Datei ordnen und straffen
  (Sichten → Trennen → Versorgen → Verdichten), als Vorstufe zum Ordnen und Aufräumen des
  ganzen Repos.

## Architektur

### Ziele

- Den Startpfad klein und testbar halten.
- UI-Verdrahtung vom App-Bootstrap isolieren.
- Den Datenfluss zwischen Stores, Panels und Renderer explizit halten.
- Zweckorientierte Variablennamen und klare Schichtung, damit der Code navigierbar und selbstdokumentierend bleibt.

### Geschichtete Architektur

Die Codebasis folgt einer Architektur mit vier Schichten. Abhängigkeiten fliessen nur **nach innen** (Presentation → Application → Domain; Infrastructure wird von der Application konsumiert). Die Domain kennt nichts anderes; die Infrastructure kennt nur die Domain-Typen, die sie serialisiert.

- **Presentation:** `src/gui/`, `src/app/main_window.*` — wxWidgets-Panels, GLCanvas, Menüs, Dialoge.
- **Application:** `src/app/` (inkl. `src/app/binding/`) — Composition Root, EventBus, `main_window_binder`, wx-App-Lifecycle, Command-Callbacks.
- **Domain:** `src/packages/` — Value Objects, Stores, Transformationslogik, sigslot-Signale — UI-agnostisch und Boost-frei (kein `friend boost::serialization::access`, keine `serialize`-Member).
- **Infrastructure:** `src/graphics/`, `src/app/services/` — OpenGL-Pipeline, FreeType, XML-/CSV-/PNG-I/O, **nicht-intrusive Serialisierung**.

Der Rendering-Adapter (`calendar_page.hpp` + `calendar_scene_composer.hpp`) ist die Brücke zwischen Application und Infrastructure; beide werden in der Verzeichnisübersicht weiter unten detailliert.

### Verzeichnisübersicht

- `src/app/`
  - `main.cpp` — nur Prozesseinstiegspunkt (Application).
  - `decade_app.hpp` — wx-App-Lifecycle, Locale-Initialisierung und Command-Line-Parsen des optionalen Startup-File-Arguments (Application).
  - `app_config.hpp` — Startup-/Fenster-Defaults (Application).
  - `runtime_options.hpp` — `app::RuntimeOptions` plus `RuntimeOptionsFromEnv()`: die einzige Stelle, die die `DECADE_*`-Env-Variablen liest (Startdatei, PNG-Dumps, Auto-Exit). CLI-Argumente überschreiben die aus der Umgebung abgeleiteten Werte in `DecadeApp::OnInit` (Application).
  - `main_window.hpp` — Composition Root: besitzt alle Stores, Panels, den Renderer und den anwendungsweiten `LocaleDateFormatter` via PIMPL (`struct Impl`) (Presentation/Application-Grenze). Der Formatter wird hier einmal konstruiert und an alle Konsumenten per Referenz weitergegeben (Datentabelle, CSV-I/O), damit die Locale-Konfiguration an einer Stelle bleibt.
  - `services/project_io.hpp` — XML-Projektdatei-Persistenz (`LoadProjectXml`/`SaveProjectXml`) (Infrastructure). UI-agnostisch, nimmt Stores per Referenz; serialisiert deren Value Objects und schreibt sie beim Laden über `Receive*` / `SetValue` zurück.
  - `services/csv_io.hpp` — CSV-Import/Export für Datumseinträge (Infrastructure). wx-frei und unit-getestet; nimmt ein `LocaleDateFormatter&`. Das ist eine benutzerseitige Grenze: CSV-zu-Daten sind inklusiv, werden beim Lesen in das interne halb-offene Intervall umgerechnet (`PeriodFromInclusiveDates`) und beim Schreiben mit `Last()` zurückgeführt.
  - `services/runtime_info.hpp` — `PrintRuntimeInfo`: Startdiagnostik (Compiler-/OS-/Toolkit-Versionen). Separat von den Persistenzdiensten gehalten, weil es die wx-Platform-Header benötigt.
  - `services/value_serialization.hpp` — nicht-intrusive, Boost-Serialization-freie Funktionen für jeden Domain-Value-Type (Infrastructure). Arbeitet nur über öffentliche APIs; besitzt das On-Disk-Format. Hält die Domain-Schicht Boost-frei.
  - `binding/event_bus.hpp` — typisierter Signal-Hub für Domain-Ereignisse (Application).
  - `binding/main_window_binder.hpp` — verdrahtet Produzenten und Konsumenten über den EventBus (Application).
  - `binding/calendar_page.hpp` — Rendering-Adapter: besitzt den kalenderrelevanten Domain-Status, stellt die `Receive*`-Slots bereit und treibt bei Updates den Scene-Builder an (Application).
  - `binding/calendar_scene_composer.hpp` — koordiniert den Aufbau des Kalender-Scene-Graphen aus dem (referenzierten) Domain-Status; GL-Canvas-frei, kennt nur `GraphicsEngine` und die `Scene` (Application/Infrastructure-Brücke). Es ist ein dünner Koordinator, der an folgende Bausteine delegiert: `calendar_layout.hpp` (`CalendarLayout`, die GL-freie Seitengeometrie, unit-getestet), `calendar_scene_nodes.hpp` (`CalendarSceneNodes` als Handle-Struct + `BuildCalendarSceneNodes`-Skeleton-Factory + die `calendar_layers`-Painter-Layer), `calendar_section_builders.hpp` (je ein freies `calendar_sections::Build*` pro Kalenderelement, jeweils mit einem `SectionContext`; `BuildBars` emittiert zusätzlich die `PickBox`es der Bars in Seitenkoordinaten), und `scene_highlighter.hpp` (`SceneHighlighter`: Hover-Farbwechsel + Scene-Tree-Selection-Overlay + `NodeWorldBounds`). Die generischen, domainfreien Node-Fill-Helper liegen in `graphics/scene_shape_filler.hpp` (`scene_shapes::FillRectangles` / `AddCenteredText`).
  - `binding/scene_snapshot.hpp` — `SceneNodeSnapshot`: ein schlichtes, GL-freies Read-Model des Render-Scene-Graphs (Name + Has-Shape + Children). Der Bus trägt es, das Scene-Tree-Panel rendert es — so hängt die Presentation-Schicht nie vom OpenGL-Typ `SceneNode` ab (Application).
  - `binding/scene_snapshot_builder.hpp` — `BuildSceneSnapshot(const SceneNode&)`: die Brücke zwischen Application und Infrastructure, die den lebenden `SceneNode`-Graphen in ein `SceneNodeSnapshot` spiegelt. Separat von `scene_snapshot.hpp` gehalten (das GL-frei bleibt) und vom Scene-Builder (dessen Aufgabe das Bauen des Graphen ist, nicht das Spiegeln).
  - `binding/interaction_controller.hpp` — übersetzt Canvas-Pointer-Bewegungen in Hover-/Pick-Ereignisse. Hit-Testing über eine austauschbare Pick-Quelle (damit es `CalendarPage`/`PhysicsWorld` nicht kennen muss) und emittiert `hovered` nur bei Änderungen (Application).
- `src/packages/` — Domain-Schicht, aufgeteilt in **Value Objects** und **Stores**. Die konzeptionelle Trennung ist auch physisch: jedes Value Object und sein Store liegen in **separaten Dateien** mit dem Namen der Hauptklasse (z. B. `date_group.hpp` + `date_group_store.hpp`, `date_entry.hpp` + `date_entry_store.hpp`, `page_setup_config.hpp` + `page_setup_store.hpp`). Der Value-Object-Header hat keine Store-Abhängigkeit; der Store-Header inkludiert seinen Value-Object-Header.
  - **Value Objects** (`Date`/`DatePeriod`, `DateGroup`/`DateGroups`, `DateEntry`, `Bar`, `PageSetupConfig`, `TitleConfig`, `ShapeConfiguration`/`ShapeConfigSet`, `CalendarSpan`/`CalendarConfig`) halten Daten plus die Abfragen darauf. Sie **kapseln ihren Zustand**: Datenmember sind `private`, werden über konstante Accessor-/Query-Methoden exponiert und nur über benannte Setter verändert — das ist die orthodoxe DDD-Form (ein Value Object ist durch seine Attribute definiert, nicht durch öffentlich sichtbare Felder), und sie hält On-Disk-Format und Invarianten an einer Stelle. Kein Signal, keine Serialisierung, kein `friend` → Rule of Zero, frei kopierbar.
  - **Stores** (`DateGroupStore`, `DateEntryStore`/`DateEntryBarStore`, `TransformDateEntry`, `PageSetupStore`, `TitleConfigStore`, `ShapeConfigurationStore`, `CalendarConfigStore`) kombinieren ein Value Object mit einem `sigslot::signal` und einer Re-Entry-Guard. Sie haben Identität → explizit nicht kopierbar. Ihr **Signal trägt den Value** (`signal<const Value&>`), sodass Konsumenten (Panels, `CalendarPage`) direkt mit kopierbaren Value Objects arbeiten; der Store stellt `Receive<Value>` / `Send<Value>` / einen `Get<Value>`-Getter bereit und hat keine eigene Query-Delegation.
  - **Muss UI-agnostisch bleiben (kein wx, kein GL) und Boost-frei** — Persistenz liegt in `services/value_serialization.hpp`.
  - **Datumslogik:** `date.hpp` (`Date`, proleptischer Gregorianischer Kalender, expliziter Invalid-State) und `date_period.hpp` (`DatePeriod`) bieten eine **datenbankfreie Schnittstelle**. Die kalenderbezogenen Berechnungen sind an ICU delegiert und bewusst auf genau zwei Header beschränkt: `detail/icu_date_backend.hpp` (Arithmetik-Backend) und `date_format.hpp` (`LocaleDateFormatter`, sprach- und lokalisierungsabhängiges Parsen und Formatieren für GUI/CSV). Einen Wechsel der Datumslibrary bedeutet, genau diese zwei Header neu zu implementieren — sonst greift niemand im Code direkt auf ICU-Datum-APIs zu. Daten werden als ISO-8601-Strings persistiert (`app::serialization_detail` in `services/value_serialization.hpp`); das frühere Boost.DateTime-basierte Projektdateiformat ist **nicht mehr lesbar** (bewusster Formatbruch).
  - **Intervallsemantik:** `DatePeriod` ist **überall im Modell einheitlich halb-offen `[begin, end)`** — `LengthDays()` ist `end - begin`, `Last()` ist der Tag vor `end`, und ein Zeitraum ohne enthaltenen Tag (`end <= begin`) ist *null*. Ein einzelner Tag ist `(d, d+1)`, Länge 1; die Stores verwerfen null Periods beim Empfang. Nutzer denken in *inklusiven* "von .. bis"-Daten, daher passiert die Umrechnung an genau zwei nutzerseitigen Grenzen — im Datumstabellen-Panel und im CSV-I/O — via `PeriodFromInclusiveDates()` beim Eingang und `Last()` bei der Anzeige. Nirgends sonst soll ±1-Tag-Arithmetik auftauchen; es soll so bleiben.
- `src/gui/` — Presentation: wxWidgets-Panels und der GL-Canvas-Wrapper. Jedes Panel besitzt seine Widgets und bietet Signale mit derselben Schnittstelle wie sein Store. `scene_tree_panel.hpp` ist eine schreibgeschützte Baumansicht des Scene-Graphs (verarbeitet `SceneNodeSnapshot`); `mouse_interaction.hpp` wandelt Drag/Wheel in Pan/Zoom für das MVP um und rechnet Pixel in Seitenraum zurück, damit Hit-Testing möglich ist. Zwei gemeinsame Helfer liegen hier: `wx_owned.hpp` (`MakeOwned<T>`, das Konstruktionsmuster für parent-owned Widgets) und `table_panel_base.hpp` (`TablePanelBase`, das Table-Plus-Add/Delete-Gerüst für die Eintrags- und Gruppen-Panels).
- `src/graphics/` — Infrastructure: OpenGL-Engine, Shader, `SceneNode`-Scene-Graph, `RectanglesShape` / `QuadrilateralShape` / `FontShape`, `RenderToTexture`, `RenderToPng`, FreeType-Wrapper. `pick_id.hpp` hält die dependency-freien Werttypen `PickId` / `PickBox`, die Scene-Builder und Picking-Layer gemeinsam nutzen.
- `src/physics/` — Infrastructure: `physics_world.hpp` (`PhysicsWorld`), ein schlanker RAII-Wrapper um einen Bullet-`btCollisionWorld`, der für 2D-Hit-Testing verwendet wird. Pickbare Bars werden als dünne achsenparallele Boxen registriert; ein Strahl durch den Seitenraum-Punkt des Cursors meldet die getroffene `PickId`. Das Collision World ist die räumliche Struktur, die eine spätere Dynamics-World (Dragging, echte Physik) erweitern kann, ohne den Picking-Pfad zu ändern.

### Schichtregeln

1. `main.cpp` darf keine Business- oder UI-Verdrahtungslogik enthalten.
2. `DecadeApp` darf High-Level-Objekte konstruieren, sollte aber Panel-/Store-Interna vermeiden.
3. `MainWindow` besitzt die Lifetime; die Verdrahtung lebt in `main_window_binder` (freie Funktionen in `src/app/binding/`).
4. `packages/` (Domain) muss UI-agnostisch bleiben — kein wx, kein GL.
5. Domain-Stores publizieren ihren Zustand über den `EventBus`; Konsumenten abonnieren über den Bus statt über einzelne Store-Signale.
6. Infrastructure-Module nehmen Domain-Typen per Referenz; sie dürfen nicht von Presentation abhängen.

### Ereignisfluss (EventBus via sigslot)

Die komponentenübergreifende Kommunikation läuft über einen in-process **EventBus** (siehe `src/app/binding/event_bus.hpp`). Der Bus besitzt pro Domain-Ereignis ein typisiertes `sigslot::signal`. Panels und Stores behalten ihre eigenen `Send*`- / `Receive*`-Methoden, aber die Verdrahtung ist zentral in `main_window_binder::Bind` — jeder Produzent wird in den Bus weitergeleitet, jeder Konsument vom Bus abonniert.
`MainWindow::EstablishConnections` ist inzwischen nur noch ein dünner Wrapper, der `main_window_binder::Bind(...)` gefolgt von `SendInitialValues(...)` aufruft.

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

Wenn du einen neuen Zustand hinzufügst: Erzeuge ein Boost-freies **Value Object** plus einen `*Store` darum in `packages/`, ein Panel in `gui/`, füge ein typisiertes Signal zu `EventBus` hinzu und registriere beide Enden in `main_window_binder::Bind`. Wenn der Zustand persistiert wird, ergänze nicht-intrusive `save`/`load` (oder `serialize`) für den Value in `services/value_serialization.hpp` und eine Zeile in `project_io`. `CalendarPage` braucht nur dann einen `Receive*`-Slot, wenn sich der Zustand auf das Rendering auswirkt.

### OpenGL-Initialisierung

Verzögert: `MainWindow` ruft `GLCanvas::InitOpenGL(version, callback)` auf. Der Callback läuft **nachdem** der GL-Context aktuell ist — dort wird `CalendarPage` konstruiert und `main_window_binder::Bind` ausgeführt. Vor diesem Callback darf kein GL-Zustand angefasst werden.

### Weiterführende Refactoring-Ziele

1. Stores sollen direkt in den `EventBus` publizieren, sobald alle Konsumenten bus-only sind.
2. Refactoring-Notizen sollen in dieser Datei bleiben, bis eine Entscheidung stabil genug für ein eigenes Dokument ist.
3. Das aktuelle Verhalten soll erhalten bleiben, während die Anzahl der Stellen sinkt, die von einem bestimmten Zustand wissen müssen.

### Designkriterien

- Intention-revealing names: Namen sollen den Zweck beschreiben, nicht den Mechanismus.
- Principle of Least Astonishment: Namen, Signaturen und Verhalten sollen zusammenpassen.
- Die kleinste nützliche Abstraktion wählen.
- Expliziten Datenfluss vor versteckter Kopplung bevorzugen.
- Stabile Regeln von instabilem Backlog trennen.
- Kommentare für das Warum, nicht für das Was.
- Wissensduplikation reduzieren, nicht jede ähnlich aussehende Zeile.

**Erledigt** (zur Nachvollziehbarkeit, nicht mehr offen):

- ~~Style-Migration auf Google C++ Style~~ — Klassen-Datenmember tragen den Trailing-Underscore; vom Gate erzwungen via `readability-identifier-naming` (siehe `.clang-tidy` und die Naming/Style-Notiz unter [Konventionen](#konventionen)).
- ~~Datei- und Naming-Struktur in `packages/`~~ — Value-Object und Store je eigene Datei (nach Hauptklasse benannt), Store-Suffix einheitlich `…Store`; auch die Member-/Parameternamen sind auf `…_store` vereinheitlicht (kein `…_storage`).
- ~~Unit-Tests für die CSV-/XML-Konvertierung~~ — `tests/services/test_csv_io.cpp` und `tests/services/test_value_serialization.cpp` decken Lesen/Schreiben, Rundläufe und Grenzfälle ab; die CSV-/XML-Logik liegt dafür in eigenen, wx-freien Headern (`services/csv_io.hpp`, `services/value_serialization.hpp`).
- ~~`PageSetupConfig` als gekapseltes Value-Object~~ — war als einziges Value-Object ein öffentliches Aggregat; jetzt private Member mit `Size()`/`Margins()`/`Orientation()`-Accessoren und `Set*`-Settern, genau wie die übrigen Value-Objects (siehe die Value-Objects-Notiz oben). Persistenz läuft non-intrusiv über die `save`/`load`-Paarung in `services/value_serialization.hpp`.

## Entwurfsprinzipien

Das sind die verbindlichen Entwurfsprinzipien für diese Codebasis.

- Single Responsibility Principle (SRP)
- Separation of Concerns (SoC)
- Kopplung und Kohäsion
- Domain-Driven Design (DDD)
- Clean Architecture
- Don’t Repeat Yourself (DRY) — wenn dieselbe mehrzeilige Form über mehrere Methoden (oder Panels) wiederkehrt, ziehe sie in einen kleinen Helfer hoch — ein `private`-Member, eine freie Funktion oder eine gemeinsame Basisklasse — statt sie zu kopieren. Etablierte Beispiele: `scene_shapes::FillRectangles` / `AddCenteredText` (Scene-Node-Erzeugung), `GLCanvas::ReadBackBuffer` (Rendern + `glReadPixels` + Zeilenflip), `MakeOwned<T>` in `gui/wx_owned.hpp` (das `make_unique<T>(...).release()`-Muster für parent-owned Widgets), `TablePanelBase` in `gui/table_panel_base.hpp` (das Tabellen- plus Add/Delete-Gerüst, das von den Entries- und Groups-Panels gemeinsam genutzt wird), und `serialization_detail::ColorToArray` / `ColorFromArray` (glm::vec4-zu-Array-Marshalling). Bevorzuge dies gegenüber Makros, weil Makros Lesbarkeit und Debuggability verschlechtern — die expliziten, feldweisen `save`/`load`-Paare in `services/value_serialization.hpp` bleiben absichtlich ausgeschrieben, weil sie das On-Disk-Format dokumentieren.

### GRASP-Muster

Wende die GRASP-Heuristiken zur Verantwortungszuweisung an, wenn du entscheidest, wo Code hingehört:

- Information Expert
- Creator
- Controller
- Low Coupling / High Cohesion
- Indirection
- Pure Fabrication
- Polymorphism / Protected Variations

### Geschichtete Architektur

Beachte die vier Schichten und die inwards-only-Abhängigkeitsregel aus den [Schichtregeln](#schichtregeln). Neuer Code muss angeben, zu welcher Schicht er gehört, und deren Abhängigkeitsgrenzen einhalten.

## Konventionen

- C++23, keine Compiler-Erweiterungen.
- Header-Guards benutzen den Dateinamenstil: der grossgeschriebene Dateiname mit dem Punkt vor dem Suffix als `_`, z. B. `main_window.hpp` → `MAIN_WINDOW_HPP`, `opengl_panel.hpp` → `OPENGL_PANEL_HPP`. Kein Verzeichnispfadpräfix. Das konsequent in `#ifndef`, `#define` und dem abschliessenden `#endif  // <GUARD>`-Kommentar anwenden. (Die clang-tidy-Prüfung `llvm-header-guard`, die sonst einen Full-Path-Stil wie `HOME_TITAN99_CODE_DECADE_SRC_..._HPP` erzwingen würde, ist in `.clang-tidy` deaktiviert — so lassen.)
- **Naming/Style — Konvention: Google C++ Style** (in Kraft):
  - Typen: `PascalCase` (`DateGroup`).
  - Funktionen & Methoden: `PascalCase` (`GetDateGroups()`); triviale Accessor/Mutator dürfen `snake_case` wie ihr Member heissen (`set_count()`).
  - Klassen-Datenmember: `snake_case` **mit Trailing-Underscore** (`date_format_`). Struct-Member ohne Underscore. Diese Member-Regel wird vom clang-tidy-Gate erzwungen (`readability-identifier-naming` in `.clang-tidy`); ein Member ohne Underscore bricht den Build.
  - Locals: `snake_case`. Konstanten/Enumeratoren: `kPascalCase` (`kColorScale`).
  - Der Store-Suffix ist einheitlich `…Store` (kein `…Storage`) — bei Typen **und** bei Member-/Parameternamen (`…_store`, nicht `…_storage`).
  - Renames zur Vereinheitlichung von Schreibweise und Bezeichnern sind erwünscht. Wenn umbenannt wird, dann **vollständig und konsistent** über alle Vorkommen (Deklaration, Definition, Aufrufstellen, Tests, Doku) — kein halber Rename, der zwei Schreibweisen nebeneinander stehen lässt. Den Build danach grün halten (Compile + `ctest` + clang-tidy-Gate).
- Rule of Five: Klassen mit expliziten Destruktoren sollten Copy/Move ebenfalls löschen oder defaulten (siehe `MainWindow`, `DateEntryStore`).
- Niemals rohes `new`/`delete` verwenden. Ownership immer über Smart Pointer ausdrücken (`std::unique_ptr` für exklusive Ownership, `std::shared_ptr` für geteilte Ownership), damit die Lifetime im Typsystem kodiert ist, Ausnahmen keine Ressourcen leaken und Ownership-Übergaben an Call-Sites explizit sind. wx-Widgets werden typischerweise via `.release()` an wx-owned Parents übergeben (wx besitzt dann die Lifetime).
- **Raw-Pointer-Datenmember sind verboten** — auch nicht-ownende. Ein rohes `T*`-Member kodiert weder Ownership noch Lifetime und ist die klassische Dangling-Pointer-Falle. Drücke die Beziehung stattdessen im Typsystem aus:
  - **Nicht-ownende Referenz auf ein von `wxTrackable` abgeleitetes Objekt** (wx-owned Parents/Children — `wxWindow`, `wxEvtHandler` und die meisten wx-Klassen): `wxWeakRef<T>` verwenden. Es setzt sich automatisch auf null zurück, wenn das referenzierte Objekt zerstört wird. Siehe [wxWeakRef](https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html) und [wxTrackable](https://docs.wxwidgets.org/3.2/classwx_trackable.html).
  - **Nicht-ownende Referenz auf ein wx-owned Objekt, das *nicht* `wxTrackable` ist** (z. B. `wxPGProperty` und seine Subklassen erben von `wxObject`, daher kompiliert `wxWeakRef` nicht): **gar keinen Pointer cachen**. Das Objekt bei Bedarf vom Owner holen — aus dem Event, das es trägt (`wxPropertyGridEvent::GetProperty()`), oder per Namenssuche am Owner (`wxPropertyGrid::GetPropertyByName()`) — mit einer stabilen Namenskonstante, die Erzeugung und Lookup gemeinsam verwenden. Der temporäre `T*`, den eine wx-API am Call-Site zurückgibt, ist in Ordnung; das Speichern als Member nicht.
  - **Ein Heap-Objekt besitzen**: ein Smart Pointer (`std::unique_ptr` / `std::shared_ptr`) oder `MakeOwned<T>`, wenn ein wx-Widget an seinen wx-owned Parent übergeben wird.

## Werkzeuge

### clang-tidy

**Durchsetzungs-Gate (Build-Brecher).** Das Gate führt clang-tidy auf der einzelnen Translation Unit `src/app/main.cpp` aus — die transitiv alle `src/`-Header via `HeaderFilterRegex` abdeckt — und stuft jedes Finding als Error ein:

```bash
cmake --build build --target clang-tidy   # schlägt bei JEDEM Finding fehl
```

 Der Baum wird bei **null Findings** gehalten; ein neues Finding bricht dieses Target. Das Gate ist bewusst *nicht* Teil des Standard-Builds, damit normale Compiles schnell bleiben — explizit oder in CI ausführen. Bevorzuge diese Single-TU-Form gegenüber direktem Globbing von `src/**/*.hpp`: Header isoliert zu analysieren erzeugt künstliches `misc-include-cleaner`-Rauschen für die GL-/wx-Umbrella-Header, das in einer echten Translation Unit nie auftritt. Die in `.clang-tidy` deaktivierten Checks tragen jeweils einen Kommentar, der erklärt, warum (glm-Unions, GL-/wx-C-API-Interop, absichtlicher Stil); siehe auch die Unterdrückungsregel unter [Build-Prüfungen](#build-prüfungen).

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

```bash
find . -regex '.*\.\(cpp\|cxx\|hpp\|cc\|h\)' -not -path './build/*' -not -path './external/*' -exec clang-format -style=file -i {} +
```

# CLAUDE.md — decade

## Projekt
decade ist ein C++23-Projekt (CMake, wxWidgets, Boost, OpenGL). Es ist aus
einem Prototyp gewachsen und trägt technische Schulden: Spaghetti-Strukturen,
Gott-Klassen, unklare Besitzverhältnisse. Ziel ist **evolutionäres Refactoring**
— schrittweise, verhaltenserhaltend, ohne grossen Rewrite.

## Grundhaltung bei Änderungen
- **Sicherheit vor Struktur.** Bevor eine strukturelle Änderung an einer
  Gott-Klasse oder einem verworrenen Pfad beginnt, muss eine Blackbox-Absicherung
  stehen (Characterization Test: Input rein, Ist-Output als Erwartung
  festschreiben). Kein Umbau ohne diese Absicherung.
- **Verhalten nicht ändern, während aufgeräumt wird.** Aufräumen (Formatierung,
  Umbenennung, Warnungsbeseitigung) und Verhaltensänderung sind *getrennte*
  Commits. Beim „Reparieren" einer Warnung nie stillschweigend die Semantik
  ändern.
- **Kleine, umkehrbare Schritte.** Ein Commit = eine Absicht. Grosse gemischte
  Diffs vermeiden.
- Reine Formatierungscommits isolieren und in `.git-blame-ignore-revs`
  eintragen, damit die Historie lesbar bleibt.

## Besitzverhältnisse (Ownership)
Der Kern der Schulden hier ist unklarer Besitz, nicht fehlende Kommentare.
- **Besitz explizit machen.** RAII durchgehend; kein manuelles `new`/`delete` in
  neuem oder berührtem Code.
- Reihenfolge der Wahl: Wertsemantik / Stack / Container zuerst → `unique_ptr`
  für exklusiven Besitz → `shared_ptr` *nur*, wenn Besitz nachweislich geteilt
  ist (ein Design-Signal, kein Default) → `weak_ptr` gegen Zyklen.
- Rohzeiger nur als nicht-besitzende Beobachter, nie als Besitzer.

## Code-Ästhetik: selbstdokumentierend
Ziel: Der Code kommuniziert seine Absicht selbst; Prosa ist minimal.
- **Intention-revealing names**: Namen nennen den Zweck, nicht die Mechanik.
- Kleine, fokussierte Funktionen mit einer Verantwortung.
- Kommentare erklären das **Warum** (Entscheidung, Trade-off, Nicht-Offensichtliches),
  nie das Was. Ein Kommentar, der beschreibt, *was* der Code tut, ist ein Hinweis,
  den Code klarer zu machen — nicht, den Kommentar zu behalten.
- **Principle of Least Astonishment**: Verhalten entspricht dem, was Signatur und
  Name erwarten lassen.

## Wissen nicht duplizieren: SSOT
- Massgeblich ist **Single Source of Truth**: Jedes Stück Wissen (Regel,
  Konstante, Domänenentscheidung) hat genau eine autoritative Repräsentation.
- „DRY" hier im Originalsinn verstehen: Duplikation von *Wissen/Absicht*
  vermeiden — nicht mechanisch jeden gleich aussehenden Codeblock zusammenlegen.
- Zwei zufällig identische Blöcke, die *verschiedene* Konzepte ausdrücken,
  bleiben getrennt. Duplikation ist billiger als die falsche Abstraktion; im
  Zweifel nicht verfrüht abstrahieren.
- **const-correctness** konsequent: SSOT für Veränderlichkeit — was nicht
  verändert wird, ist `const`.

## C++-Spezifika
- **Header-Hygiene**: Include-Dickicht auflösen (Vorwärtsdeklarationen, minimale
  Includes, include-what-you-use). Grösserer struktureller Hebel als
  Reformatierung, senkt nebenbei Compile-Zeiten.
- **Warnungen schrittweise**: `-Wall -Wextra` an; Warnungen als getrackte Metrik
  abbauen. `-Werror` nicht sofort projektweit, sondern für bereinigte Module /
  neu berührten Code scharfschalten.
- `clang-format` / `clang-tidy` als konsistenter Style; Änderungen daraus als
  eigene Commits (siehe oben).

## Was zu vermeiden ist
- Strukturelle Umbauten ohne vorherige Test-Absicherung.
- Verhaltensänderung vermischt mit Aufräumen im selben Commit.
- Massen-Reformatierung vermischt mit Logikänderung.
- Spekulative Abstraktion / verfrühtes DRY über Konzeptgrenzen hinweg.
- `shared_ptr` als Default statt als bewusstes Besitz-Signal.

## Stil

- Kommentare und Doku: knappes, aktives Deutsch (Wolf Schneider). So wenige Zeilen wie möglich; nur Nicht-Offensichtliches kommentieren.
- **Echte Umlaute schreiben** (ä/ö/ü), nie Ersatzschreibweisen (ae/oe/ue). Schweizer Rechtschreibung: ss statt ß.

