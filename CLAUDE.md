# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) and other coding
agents when working with code in this repository.

## Project

C++23 desktop calendar/timeline application. wxWidgets GUI, OpenGL (4.6 core) rendering via libepoxy, ICU for calendar arithmetic, locale date parsing/formatting and Unicode text handling, Boost.Serialization for XML project files, FreeType for text, csv2 for CSV import/export, sigslot for signal/slot wiring.

## Build & Run

The CMake build directory is `build/` (Ninja generator, with `compile_commands.json` exported for clangd/clang-tidy).

```bash
# Reconfigure (only when CMakeLists.txt or dependencies change)
cmake -S . -B build -G Ninja

# Build (run from repo root; the ninja file lives in build/)
ninja -C build

# Run the GUI
./build/decade
```

### Startup file

A project/data file can be opened at startup via a positional argument — the
idiomatic "open with file" entry point. The path is resolved against the current
working directory, so it works regardless of where the binary lives:

```bash
./build/decade test-files/test_dates_1.csv   # CSV import
./build/decade myproject.xml                  # XML project (dispatched by extension)
```

Loading is **opt-in**: with no argument and no `DECADE_DEFAULT_CSV` set, the app
starts with an empty project. The startup-file resolution (CLI argument and the
env-var fallback) is centralised in `src/app/runtime_options.hpp`
(`app::RuntimeOptions`).

### Headless / scripted runs

The binary honors several environment variables (read in
`src/app/runtime_options.hpp`) for non-interactive use — CI, screenshots, smoke
tests.

**Image capture** — three variables capture three different things; they are not
redundant:

| Variable | Captures | Resolution / look | Needs |
| --- | --- | --- | --- |
| `DECADE_DUMP_PNG=<path>` | the calendar **page artwork** only, via an off-screen FBO | export DPI, white background, no app chrome | OpenGL only |
| `DECADE_DUMP_WINDOW_PNG=<path>` | the **GL canvas pane** exactly as on screen (`glReadPixels` on the back buffer) | screen resolution, dark margins around the page | OpenGL only — works on Wayland |
| `DECADE_DUMP_FRAME_PNG=<path>` | the **whole frame**: tabs + panels (`wxClientDC` blit) with the GL back buffer composited on top | screen resolution | widget read-back needs X11/Xvfb (blank under Wayland) |

`DUMP_WINDOW_PNG` is the canvas-only subset of `DUMP_FRAME_PNG`; prefer it when
you only need the rendered canvas (and to avoid Xvfb), and `DUMP_PNG` when you
need a clean high-DPI export of the page itself. All dumps are deferred via
`CallAfter` so the first paint has happened.

**Control:**

- `DECADE_DUMP_PNG_DPI=<dpi>` — export DPI for `DECADE_DUMP_PNG`; defaults to `GLCanvas::kExportPngDpi` (200) when unset. Used e.g. for high-resolution README renders.
- `DECADE_EXIT_AFTER_MS=<ms>` — auto-close the main window after N ms.
- `DECADE_SELECT_TAB=<label>` — pre-select a notebook tab by label (case-insensitive), e.g. for screenshotting a specific tab.
- `DECADE_DEBUG_LOG=1` — enable OpenGL/runtime debug logging.
- `DECADE_DEFAULT_CSV=<path>` — opt-in startup file (CSV or XML) when no positional argument is given; the CLI argument takes precedence. No file is loaded if this is unset.

Typical smoke test (pass the sample data explicitly):
```bash
DECADE_DUMP_PNG=/tmp/decade_render.png DECADE_EXIT_AFTER_MS=2000 \
  stdbuf -oL -eL timeout 12 ./build/decade test-files/test_dates_1.csv
```

Full-UI screenshot (tabs + panels + canvas) of a specific tab. The widget
read-back only works on the **X11 backend** — a `wxClientDC` blit returns black
under Wayland — so run it headless under **Xvfb** with software GL. This is the
supported way to screenshot the actual GUI: GNOME/Wayland blocks programmatic
screen capture, and forcing `GDK_BACKEND=x11` on a live XWayland session breaks
the GL canvas' EGL surface.
```bash
xvfb-run -a -s "-screen 0 1600x1000x24" \
  env GDK_BACKEND=x11 LIBGL_ALWAYS_SOFTWARE=1 \
  DECADE_SELECT_TAB=Timeframe DECADE_DUMP_FRAME_PNG=/tmp/decade_ui.png \
  DECADE_EXIT_AFTER_MS=3000 timeout 30 ./build/decade test-files/test_dates_1.csv
```

## Build hygiene

- **Warnings break the build — fix them, don't suppress. This rule applies to
  BOTH compiler warnings AND clang-tidy diagnostics.** Do not silence a finding
  with `NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN`, a `#pragma`, a `-Wno-…`
  flag, or a `.clang-tidy` per-line exclusion just to make it quiet. Change the
  code so the finding no longer applies. Restructuring, RAII, and proper type
  annotations are fixes; suppression is not.
  - **Prefer a real fix even when it looks like the warning is "unfixable."**
    Most are not. Example: `cppcoreguidelines-owning-memory` over a raw C
    resource is satisfied by expressing ownership with `gsl::owner<>` (the
    project already links `Microsoft.GSL::GSL`) and/or a `std::unique_ptr` with
    a custom deleter — see `src/graphics/png_writer.hpp`. A check that is
    genuinely inapplicable to the project's style is disabled **once, globally**
    in `.clang-tidy` with a comment (as `-modernize-use-trailing-return-type`,
    `-llvm-header-guard`, … already are) — never scattered per-line.
  - **Suppression is a last resort, allowed only for a construct a third-party
    C API contractually forces on us and that has no in-code fix.** The
    canonical (and currently only sanctioned) example is libpng's mandatory
    `setjmp`/`longjmp` error handling in `src/graphics/png_writer.hpp`. When you
    must suppress: scope the `NOLINT` to the **specific check names** (never a
    bare `NOLINT`), keep it to the narrowest line, and add a comment stating
    *why* it cannot be fixed. If a whole dependency makes a class of warnings
    unavoidable, prefer replacing that dependency over spreading suppressions.
- `.clang-format` is the source of truth for formatting.
- CI workflow is currently disabled (`.github/workflows/cmake.yml.disable`).

## Submodules

`external/embed-resource`, `external/sigslot`, `external/csv2` are git submodules — initialize with `git submodule update --init --recursive` after clone. Shaders and licenses are embedded into the binary via `embed_resources()` in `CMakeLists.txt`.

## Header-only by design

The codebase is **header-only by design** (`main.cpp` is the only translation unit). When adding code, prefer extending headers in place over splitting into `.cpp`. Keep this convention even during refactors. Definitions that live in a header must be `inline` (free functions and out-of-class member definitions), so the single-TU rule does not silently mask ODR violations if a header is ever included from elsewhere (e.g. tests).

## Architecture

### Goals

- Keep the startup path small and testable.
- Isolate UI wiring from app bootstrap.
- Keep data flow explicit between stores, panels, and renderer.

### Layered architecture

The codebase follows a four-layer architecture. Dependencies only flow
**inward** (Presentation → Application → Domain; Infrastructure is consumed by
Application). Domain knows about nothing else; Infrastructure knows only about
the Domain types it serialises.

- **Presentation:** `src/gui/`, `src/app/main_window.*` — wxWidgets panels, GLCanvas, menus, dialogs.
- **Application:** `src/app/` (incl. `src/app/binding/`) — composition root, EventBus, `main_window_binder`, wx app lifecycle, command callbacks.
- **Domain:** `src/packages/` — value objects, stores, transformation logic, sigslot signals — UI-agnostic **and Boost-free** (no `friend boost::serialization::access`, no `serialize` members).
- **Infrastructure:** `src/graphics/`, `src/app/services/` — OpenGL pipeline, FreeType, XML/CSV/PNG I/O, **non-intrusive serialization**.

The rendering adapter (`calendar_page.hpp` + `calendar_scene_builder.hpp`) is the
Application/Infrastructure bridge; both are detailed in the directory map below.

### Directory map

- `src/app/`
  - `main.cpp` — process entrypoint only (Application).
  - `decade_app.hpp` — wx app lifecycle, locale initialisation, and command-line parsing (the optional positional startup-file argument) (Application).
  - `app_config.hpp` — startup/window defaults (Application).
  - `runtime_options.hpp` — `app::RuntimeOptions` plus `RuntimeOptionsFromEnv()`: the single place that reads the `DECADE_*` env vars (startup file, PNG dumps, auto-exit). CLI arguments override the env-derived values in `DecadeApp::OnInit` (Application).
  - `main_window.hpp` — composition root: owns all stores, panels, the renderer, and the application-wide `LocaleDateFormatter` via PIMPL (`struct Impl`) (Presentation/Application boundary). The formatter is constructed once here and handed by reference to every consumer (date table panel, CSV I/O) so the locale configuration lives in one place.
  - `services/project_io.hpp` — XML project-file persistence (`LoadProjectXml`/`SaveProjectXml`) (Infrastructure). UI-agnostic, takes stores by reference; serializes the stores' value objects and pushes them back via `Receive*` / `SetValue` on load.
  - `services/csv_io.hpp` — CSV import/export for date entries (Infrastructure). wx-free and unit-tested; takes a `LocaleDateFormatter&`. This is a user-facing boundary: CSV to-dates are inclusive, converted to the internal half-open period on read (`PeriodFromInclusiveDates`) and back on write (`Last()`).
  - `services/runtime_info.hpp` — `PrintRuntimeInfo`: startup diagnostics (compiler/OS/toolkit versions). Kept apart from the persistence services because it pulls in the wx platform headers.
  - `services/value_serialization.hpp` — non-intrusive Boost.Serialization free functions for every domain value type (Infrastructure). Goes through public APIs only; owns the on-disk format. Keeps the domain layer Boost-free.
  - `binding/event_bus.hpp` — typed signal hub for domain events (Application).
  - `binding/main_window_binder.hpp` — wires producers/consumers via the EventBus (Application).
  - `binding/calendar_page.hpp` — rendering adapter: owns the calendar-relevant domain state, exposes the `Receive*` slots, and drives the scene builder on updates (Application).
  - `binding/calendar_scene_builder.hpp` — builds and fills the calendar scene graph from the (referenced) domain state; GL-canvas free, knows only `GraphicsEngine` and the scene graph (Application/Infrastructure bridge).
- `src/packages/` — Domain layer, split into **value objects** and **stores**. The
  conceptual split is also physical: each value object and its store live in
  **separate files** named after the primary class (e.g. `date_group.hpp` +
  `date_group_store.hpp`, `date_entry.hpp` + `date_entry_store.hpp`,
  `page_setup_config.hpp` + `page_setup_store.hpp`). The value-object header carries
  no store dependency; the store header includes its value-object header.
  - **Value objects** (`Date`/`DatePeriod`, `DateGroup`/`DateGroups`, `DateEntry`, `Bar`, `PageSetupConfig`, `TitleConfig`, `ShapeConfiguration`/`ShapeConfigSet`, `CalendarSpan`/`CalendarConfig`) hold data + the queries over it. They **encapsulate their state**: data members are `private`, exposed through const accessor/query methods and mutated only through named setters — this is the orthodox DDD form (a value object is defined by its attributes, not by exposing them as public fields), and it keeps the on-disk format and invariants in one place. No signal, no serialization, no `friend` → Rule of Zero, freely copyable.
  - **Stores** (`DateGroupStore`, `DateEntryStore`/`DateEntryBarStore`, `TransformDateEntry`, `PageSetupStore`, `TitleConfigStore`, `ShapeConfigurationStore`, `CalendarConfigStore`) compose a value object plus a `sigslot::signal` and the re-entry guard. They have identity → explicitly non-copyable. Their **signal carries the value** (`signal<const Value&>`), so consumers (panels, `CalendarPage`) work with copyable value objects directly; the store exposes `Receive<Value>` / `Send<Value>` / a `Get<Value>` getter and adds no query delegation of its own. The store suffix is uniformly `…Store` (no `…Storage`).
  - **Must remain UI-agnostic (no wx, no GL) and Boost-free** — persistence lives in `services/value_serialization.hpp`.
  - **Dates:** `date.hpp` (`Date`, proleptic Gregorian, explicit invalid state)
    and `date_period.hpp` (`DatePeriod`) expose a **date-library-free
    interface**. The calendrical computation is delegated to ICU, deliberately
    confined to exactly two headers: `detail/icu_date_backend.hpp` (arithmetic
    backend) and `date_format.hpp` (`LocaleDateFormatter`, locale-aware text
    parsing and formatting for GUI/CSV). Swapping the date library means
    reimplementing those two headers — nothing else in the codebase touches ICU
    date APIs. Dates are persisted as ISO-8601 strings
    (`app::serialization_detail` in `services/value_serialization.hpp`); the
    previous Boost.DateTime-based project-file format is **not** readable
    anymore (deliberate format break).
  - **Interval semantics:** `DatePeriod` is **uniformly half-open `[begin,
    end)`** everywhere inside the model — `LengthDays()` is `end - begin`,
    `Last()` is the day before `end`, and a period with no contained day
    (`end <= begin`) is *null*. A single day is `(d, d+1)`, length 1; the
    stores drop null periods on receipt. Users think in *inclusive* "from .. to"
    dates, so the conversion happens at exactly two user-facing boundaries — the
    date table panel and CSV I/O — via `PeriodFromInclusiveDates()` on input and
    `Last()` on display. Nowhere else does ±1 day arithmetic appear; keep it
    that way.
- `src/gui/` — Presentation: wxWidgets panels and the GL canvas wrapper. Each panel owns its widgets and exposes signals matching its store's interface.
- `src/graphics/` — Infrastructure: OpenGL engine, shaders, `SceneNode` scene graph, `RectanglesShape` / `QuadrilateralShape` / `FontShape`, `RenderToTexture`, `RenderToPng`, FreeType wrapper.

### Layering rules

1. `main.cpp` must not contain business or UI wiring logic.
2. `DecadeApp` may construct high-level objects but should avoid panel/store internals.
3. `MainWindow` owns lifetime; wiring lives in `main_window_binder` (free functions in `src/app/binding/`).
4. `packages/` (Domain) must remain UI-agnostic — no wx, no GL.
5. Domain stores publish state via the `EventBus`; consumers subscribe via the bus rather than connecting to individual store signals.
6. Infrastructure modules take Domain types by reference; they must not depend on Presentation.

### Event flow (EventBus via sigslot)

Cross-component communication uses an in-process **EventBus** (see
`src/app/binding/event_bus.hpp`). The bus owns one typed `sigslot::signal` per
domain event. Panels and stores keep their own `Send*` / `Receive*` methods, but
the wiring is centralised in `main_window_binder::Bind` — each producer is
forwarded into the bus, each consumer subscribes from the bus.
`MainWindow::EstablishConnections` is now only a thin wrapper that calls
`main_window_binder::Bind(...)` followed by `SendInitialValues(...)`.

```
Panel ──Send──▶ EventBus.<event> ──▶ Store.Receive
Store ──Send──▶ EventBus.<event> ──▶ Panel.Receive , CalendarPage.Receive , GLCanvas.Receive
```

The `TransformDateEntry` adapter sits between `date_entries`
(input) and `transformed_date_entries` (output) on the bus.

When adding a new piece of state: create a Boost-free **value object** plus a
`*Store` wrapping it in `packages/`, a panel in `gui/`, add a typed signal to
`EventBus`, and register both ends in `main_window_binder::Bind`. If the state
is persisted, add non-intrusive `save`/`load` (or `serialize`) for the value in
`services/value_serialization.hpp` and a line in `project_io`. `CalendarPage`
only needs the `Receive*` slot if the state affects rendering.

### OpenGL initialization

Deferred: `MainWindow` calls `GLCanvas::InitOpenGL(version, callback)`. The
callback runs **after** the GL context is current — that is where `CalendarPage`
is constructed and `main_window_binder::Bind` runs. Do not touch GL state before
this callback fires.

### Follow-up refactor targets

1. Promote stores to publish directly into `EventBus` (removing their internal signals) once all consumers are bus-only.

**Erledigt** (zur Nachvollziehbarkeit, nicht mehr offen):

- ~~Style-Migration auf Google C++ Style~~ — Klassen-Datenmember tragen den
  Trailing-Underscore; vom Gate erzwungen via `readability-identifier-naming`
  (siehe `.clang-tidy` und die Naming/Style-Notiz unter [Conventions](#conventions)).
- ~~Datei- und Naming-Struktur in `packages/`~~ — Value-Object und Store je eigene
  Datei (nach Hauptklasse benannt), Store-Suffix einheitlich `…Store`; auch die
  Member-/Parameternamen sind auf `…_store` vereinheitlicht (kein `…_storage`).
- ~~Unit-Tests für die CSV-/XML-Konvertierung~~ — `tests/services/test_csv_io.cpp`
  und `tests/services/test_value_serialization.cpp` decken Lesen/Schreiben,
  Rundläufe und Grenzfälle ab; die CSV-/XML-Logik liegt dafür in eigenen,
  wx-freien Headern (`services/csv_io.hpp`, `services/value_serialization.hpp`).

## Design principles

These are the binding design guidelines for this codebase.

- Single Responsibility Principle (SRP)
- Separation of Concerns (SoC)
- Coupling and cohesion
- Domain-Driven Design (DDD)
- Clean Architecture

### GRASP patterns

Apply the GRASP responsibility-assignment heuristics when deciding where code
belongs:

- Information Expert
- Creator
- Controller
- Low Coupling / High Cohesion
- Indirection
- Pure Fabrication
- Polymorphism / Protected Variations

### Layered architecture

Respect the four layers and the inward-only dependency rule stated under
[Layering rules](#layering-rules). New code must declare which layer it belongs
to and obey that layer's dependency constraints.


## Conventions

- C++23, no compiler extensions.
- Header guards use the filename style: the uppercased file name with the dot before the suffix as `_`, e.g. `main_window.hpp` → `MAIN_WINDOW_HPP`, `opengl_panel.hpp` → `OPENGL_PANEL_HPP`. No directory path prefix. Apply this consistently in `#ifndef`, `#define`, and the trailing `#endif  // <GUARD>` comment. (The clang-tidy `llvm-header-guard` check, which would otherwise impose a full-path style like `HOME_TITAN99_CODE_DECADE_SRC_..._HPP`, is disabled in `.clang-tidy` — keep it disabled.)
- **Naming/Style — Konvention: Google C++ Style** (in Kraft):
  - Typen: `PascalCase` (`DateGroup`).
  - Funktionen & Methoden: `PascalCase` (`GetDateGroups()`); triviale
    Accessor/Mutator dürfen `snake_case` wie ihr Member heissen (`set_count()`).
  - Klassen-Datenmember: `snake_case` **mit Trailing-Underscore** (`date_format_`).
    Struct-Member ohne Underscore. Diese Member-Regel wird vom clang-tidy-Gate
    erzwungen (`readability-identifier-naming` in `.clang-tidy`); ein Member ohne
    Underscore bricht den Build.
  - Locals: `snake_case`. Konstanten/Enumeratoren: `kPascalCase` (`kColorScale`).
  - Der Store-Suffix ist einheitlich `…Store` (kein `…Storage`) — bei Typen
    **und** bei Member-/Parameternamen (`…_store`, nicht `…_storage`).
  - Renames zur Vereinheitlichung von Schreibweise und Bezeichnern sind
    erwünscht. Wenn umbenannt wird, dann **vollständig und konsistent** über
    alle Vorkommen (Deklaration, Definition, Aufrufstellen, Tests, Doku) — kein
    halber Rename, der zwei Schreibweisen nebeneinander stehen lässt. Den Build
    danach grün halten (Compile + `ctest` + clang-tidy-Gate).
- Rule-of-five: classes with explicit destructors should also delete or default copy/move (see `MainWindow`, `DateEntryStore`).
- Never use raw `new`/`delete`. Always express ownership through smart pointers (`std::unique_ptr` for unique ownership, `std::shared_ptr` for shared ownership) so lifetime is encoded in the type system, exceptions cannot leak resources, and ownership transfer is explicit at call sites. wx widgets are typically transferred via `.release()` to wx-owned parents (wx then owns the lifetime).
- For non-owning references to objects whose lifetime is managed by wxWidgets (wx-owned parents/children, or any class derived from `wxTrackable` — which includes `wxWindow`, `wxEvtHandler`, and most wx classes), prefer `wxWeakRef<T>` over raw pointers. It auto-nulls when the referenced object is destroyed, eliminating dangling-pointer bugs. See [wxWeakRef](https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html) and [wxTrackable](https://docs.wxwidgets.org/3.2/classwx_trackable.html).

## Tooling

### clang-tidy

**Enforcement gate (build-breaker).** The gate runs clang-tidy on the single
translation unit `src/app/main.cpp` — which transitively covers every `src/`
header via `HeaderFilterRegex` — with every finding promoted to an error:

```
cmake --build build --target clang-tidy   # fails on ANY finding
```

The tree is kept at **zero findings**; introducing one breaks this target. The
gate is deliberately *not* part of the default build, so normal compiles stay
fast — run it explicitly or in CI. Prefer this single-TU form over globbing
`src/**/*.hpp` directly: analysing headers in isolation produces spurious
`misc-include-cleaner` noise for the GL/wx umbrella headers that never appears in
a real translation unit. The disabled checks in `.clang-tidy` each carry a
comment explaining why (glm unions, GL/wx C-API interop, deliberate style); see
also the suppression policy under [Build hygiene](#build-hygiene).

Voller Lauf (Bericht in `build/clang-tidy.log`):
```
clang-tidy -p build \
  --extra-arg=-include --extra-arg=type_traits \
  --extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option \
  src/**/*.cpp src/**/*.hpp 2>&1 | tee build/clang-tidy.log
```

Auto-Fix für eine einzelne Check-Gruppe (Beispiel `modernize-*`, ohne `--fix-errors`):
```
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

```
find . -regex '.*\.\(cpp\|cxx\|hpp\|cc\|h\)' -not -path './build/*' -not -path './external/*' -exec clang-format -style=file -i {} +
```
