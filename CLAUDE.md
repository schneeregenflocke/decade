# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) and other coding
agents when working with code in this repository. It is the single, canonical
source of truth — it consolidates what used to live in `CLAUDE.md`,
`AGENTS.md`, and `docs/ARCHITECTURE.md`.

## Project

C++23 desktop calendar/timeline application. wxWidgets GUI, OpenGL (4.6 core) rendering via libepoxy, Boost.DateTime for calendar logic, Boost.Serialization for XML project files, FreeType for text, csv2 for CSV import/export, sigslot for signal/slot wiring.

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

### Headless / scripted runs

The binary honors several environment variables that are essential for non-interactive use (CI, screenshots, smoke tests):

- `DECADE_EXIT_AFTER_MS=<ms>` — auto-close the main window after N ms. Use for smoke tests.
- `DECADE_DUMP_PNG=<path>` — render the calendar page to PNG via off-screen FBO once OpenGL is ready.
- `DECADE_DUMP_WINDOW_PNG=<path>` — capture the actual window back buffer after first paint (deferred via `CallAfter`).
- `DECADE_DEBUG_LOG=1` — enable OpenGL/runtime debug logging.
- `DECADE_DEFAULT_CSV=<path>` — override the sample CSV auto-loaded at startup (default `test-files/test_dates_1.csv`).
- `DECADE_NO_DEFAULT_CSV=1` — disable auto-loading the default CSV entirely.

Typical smoke test:
```bash
DECADE_DUMP_PNG=/tmp/decade_render.png DECADE_EXIT_AFTER_MS=2000 \
  stdbuf -oL -eL timeout 12 ./build/decade
```

## Build hygiene

- **Warnings break the build** — fix them, don't suppress.
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

`src/app/binding/calendar_page.hpp` is the rendering adapter: it bridges Domain
state into the Infrastructure scene graph. It belongs to the Application layer.
The scene-graph construction itself lives in a dedicated builder,
`src/app/binding/calendar_scene_builder.hpp` (see below).

### Directory map

- `src/app/`
  - `main.cpp` — process entrypoint only (Application).
  - `decade_app.hpp` — wx app lifecycle and locale initialisation (Application).
  - `app_config.hpp` — startup/window defaults (Application).
  - `main_window.hpp` — composition root: owns all stores, panels, and the renderer via PIMPL (`struct Impl`) (Presentation/Application boundary).
  - `services/project_io.hpp` — XML/CSV/PNG persistence (Infrastructure). UI-agnostic, takes stores by reference; serializes the stores' value objects and pushes them back via `Receive*` / `SetValue` on load.
  - `services/value_serialization.hpp` — non-intrusive Boost.Serialization free functions for every domain value type (Infrastructure). Goes through public APIs only; owns the on-disk format. Keeps the domain layer Boost-free.
  - `binding/event_bus.hpp` — typed signal hub for domain events (Application).
  - `binding/main_window_binder.hpp` — wires producers/consumers via the EventBus (Application).
  - `binding/calendar_page.hpp` — rendering adapter: owns the calendar-relevant domain state, exposes the `Receive*` slots, and drives the scene builder on updates (Application).
  - `binding/calendar_scene_builder.hpp` — builds and fills the calendar scene graph from the (referenced) domain state; GL-canvas free, knows only `GraphicsEngine` and the scene graph (Application/Infrastructure bridge).
- `src/packages/` — Domain layer, split into **value objects** and **stores**:
  - **Value objects** (`DateGroup`/`DateGroups`, `DateIntervalBundle`, `PageSetupConfig`, `TitleConfig`, `ShapeConfiguration`/`ShapeConfigSet`, `CalendarSpan`/`CalendarConfig`) hold data + the queries over it. No signal, no serialization, no `friend` → Rule of Zero, freely copyable.
  - **Stores** (`DateGroupStore`, `DateIntervalBundleStore`, `PageSetupStore`, `TitleConfigStore`, `ShapeConfigurationStorage`, `CalendarConfigStorage`) compose a value object plus a `sigslot::signal` and the re-entry guard. They have identity → explicitly non-copyable. Their **signal carries the value** (`signal<const Value&>`), so consumers (panels, `CalendarPage`) work with copyable value objects directly; the store exposes `Receive<Value>` / `Send<Value>` / a `Get<Value>` getter and adds no query delegation of its own.
  - **Must remain UI-agnostic (no wx, no GL) and Boost-free** — persistence lives in `services/value_serialization.hpp`.
- `src/gui/` — Presentation: wxWidgets panels and the GL canvas wrapper. Each panel owns its widgets and exposes signals matching its store's interface.
- `src/graphics/` — Infrastructure: OpenGL engine, shaders, `SceneNode` scene graph, `RectanglesShape` / `QuadrilateralShape` / `FontShape`, `RenderToTexture`, `RenderToPng`, FreeType wrapper.
- `src/app/binding/calendar_page.hpp` / `calendar_scene_builder.hpp` — the rendering adapter (Application/Infrastructure bridge): `CalendarPage` receives store updates and owns the state; `CalendarSceneBuilder` builds the scene graph for the calendar layout and pushes shapes into `GraphicsEngine`.

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

The `TransformDateIntervalBundle` adapter sits between `date_interval_bundles`
(input) and `transformed_date_interval_bundles` (output) on the bus.

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

1. Add unit tests for CSV/XML conversion logic in `services/project_io`.
2. Promote stores to publish directly into `EventBus` (removing their internal signals) once all consumers are bus-only.

## Design principles

These are the binding design guidelines for this codebase. They formalise the
intent behind the layered architecture above; when a concrete instruction in
this file conflicts with a general principle, the concrete instruction wins.

### Single Responsibility Principle (SRP)

Every class, header, and free function should have exactly one reason to change.
A `*Store` changes when domain rules change; a panel changes when its UI layout
changes; `project_io` changes when a file format changes. The split of the
former `calendar_page.hpp` into `CalendarPage` (state + update slots) and
`CalendarSceneBuilder` (scene-graph layout) is the reference example: when a
single file accumulates several reasons to change, prefer extraction over
further growth.

### Separation of Concerns (SoC)

Keep the four concerns — presentation, application/wiring, domain logic, and
infrastructure I/O — in their respective layers. Do not let wx or GL types leak
into `packages/`, and do not embed file-format or rendering details in panels.
The EventBus exists precisely to keep producers and consumers from knowing about
each other directly.

### Coupling and cohesion

Aim for **low coupling, high cohesion**. Prefer communication through the
EventBus and Domain types over direct references between components. Members of a
class should operate on shared state toward one purpose; if a class's methods
split into disjoint groups touching disjoint fields, that is a cohesion smell and
a candidate for splitting.

### GRASP patterns

Apply the GRASP responsibility-assignment heuristics when deciding where code
belongs:

- **Information Expert** — assign a responsibility to the class that holds the
  data it needs (e.g. serialization logic stays close to the store that owns the
  state, mediated by `project_io`).
- **Creator** — the owner of an object's lifetime creates it (`MainWindow` /
  `Impl` is the composition root and creator of stores, panels, renderer).
- **Controller** — `MainWindow` / command callbacks act as the controller that
  receives UI/system events and delegates to the right collaborators.
- **Low Coupling / High Cohesion** — see above; the primary tie-breaker.
- **Indirection** — the `EventBus` and `TransformDateIntervalBundle` adapter
  decouple producers from consumers.
- **Pure Fabrication** — `main_window_binder` and `EventBus` are fabricated
  helpers that exist to keep wiring out of domain and presentation code.
- **Polymorphism / Protected Variations** — isolate variation points (shapes,
  render targets) behind stable interfaces in `src/graphics/`.

### Domain-Driven Design (DDD)

`src/packages/` is the **domain model**, with a deliberate split between **value
objects** (data + queries, Rule of Zero, copyable) and **stores** (a value plus
a `sigslot::signal` and re-entry guard; identity, non-copyable). This is the
reference example of separating value from publisher: the signal — the only
non-copyable, identity-bearing part — lives in the store, never in the value, so
values copy correctly by default and stores guard their own change
notifications. Keep the domain free of infrastructure and UI vocabulary **and
free of Boost**: persistence is non-intrusive and lives in
`services/value_serialization.hpp`. Rendering (`calendar_page`) is an adapter
around the domain, not part of it. Use the domain's own terms (intervals,
bundles, date groups, page setup) consistently in names.

### Layered architecture

Respect the four layers and the inward-only dependency rule stated under
[Layering rules](#layering-rules). New code must declare which layer it belongs
to and obey that layer's dependency constraints.

### Clean Architecture

Dependencies point inward toward the domain; the domain depends on nothing
outward. Outer layers (Presentation, Infrastructure) depend on inner layers
(Application, Domain) and never the reverse. The EventBus and the `*_binder`
free functions are the boundary-crossing mechanism that keeps this dependency
direction intact, so that the domain stays independent of wx, OpenGL, and file
formats and remains testable in isolation.

## Conventions

- C++23, no compiler extensions.
- Header guards use the filename style: the uppercased file name with the dot before the suffix as `_`, e.g. `main_window.hpp` → `MAIN_WINDOW_HPP`, `opengl_panel.hpp` → `OPENGL_PANEL_HPP`. No directory path prefix. Apply this consistently in `#ifndef`, `#define`, and the trailing `#endif  // <GUARD>` comment. (The clang-tidy `llvm-header-guard` check, which would otherwise impose a full-path style like `HOME_TITAN99_CODE_DECADE_SRC_..._HPP`, is disabled in `.clang-tidy` — keep it disabled.)
- Naming is currently mixed: `*Store` vs. `*Storage`, snake_case members like `signal_page_setup_config` vs. PascalCase `SignalDateGroups()`. When touching a file, prefer the existing local style; do not perform mass renames as a side effect.
- Rule-of-five: classes with explicit destructors should also delete or default copy/move (see `MainWindow`, `DateIntervalBundleStore`).
- Never use raw `new`/`delete`. Always express ownership through smart pointers (`std::unique_ptr` for unique ownership, `std::shared_ptr` for shared ownership) so lifetime is encoded in the type system, exceptions cannot leak resources, and ownership transfer is explicit at call sites. wx widgets are typically transferred via `.release()` to wx-owned parents (wx then owns the lifetime).
- For non-owning references to objects whose lifetime is managed by wxWidgets (wx-owned parents/children, or any class derived from `wxTrackable` — which includes `wxWindow`, `wxEvtHandler`, and most wx classes), prefer `wxWeakRef<T>` over raw pointers. It auto-nulls when the referenced object is destroyed, eliminating dangling-pointer bugs. See [wxWeakRef](https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html) and [wxTrackable](https://docs.wxwidgets.org/3.2/classwx_trackable.html).

## Tooling

### clang-tidy

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
