# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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

## Architecture

The codebase is **header-only by design** (only `main.cpp`, `decade_app.cpp`, `main_window.cpp`, `services/project_io.cpp` are translation units). When adding code, prefer extending headers in place over splitting into `.cpp`. Keep this convention even during refactors.

### Layers (see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the canonical statement)

- **`src/app/`** — process entry (`main.cpp`), wx app lifecycle (`decade_app.*`), and the composition root [main_window.cpp](src/app/main_window.cpp). `MainWindow` owns all stores, panels, and the renderer via PIMPL (`struct Impl`).
- **`src/app/services/`** — XML/CSV/PNG I/O. UI-agnostic, takes stores by reference.
- **`src/packages/`** — domain stores and config (`DateIntervalBundleStore`, `DateGroupStore`, `PageSetupStore`, `TitleConfigStore`, `ShapeConfigurationStorage`, `CalendarConfigStorage`). **Must remain UI-agnostic** (no wx, no GL). Stores emit `sigslot::signal` and serialize via Boost.
- **`src/gui/`** — wxWidgets panels. Each panel owns its widgets and exposes signals matching its store's interface.
- **`src/graphics/`** — OpenGL pipeline: `GraphicsEngine`, shaders, `SceneNode` scene graph, `RectanglesShape` / `QuadrilateralShape` / `FontShape`, `RenderToTexture`, `RenderToPng`, FreeType wrapper.
- **`src/calendar_page.hpp`** — the bridge: receives store updates, builds the scene graph for the calendar layout, and pushes shapes into `GraphicsEngine`. Currently the largest file in the codebase.

### Data flow (signal/slot via sigslot)

Wiring is performed manually in [`MainWindow::EstablishConnections`](src/app/main_window.cpp). Pattern:

```
Panel → Store (user edits)         Store → Panel       (refresh after load)
Panel ⇄ Store ⇄ CalendarPage ⇄ GLCanvas (re-render)
```

`TransformDateIntervalBundle` sits between `DateIntervalBundleStore` and `CalendarPage` for date-shift transformations.

When adding a new piece of state: create a `*Store` in `packages/`, a panel in `gui/`, and wire both directions in `EstablishConnections`. `CalendarPage` only needs the `Receive*` slot if the state affects rendering.

### OpenGL initialization

Deferred: `MainWindow` calls `GLCanvas::InitOpenGL(version, callback)`. The callback runs **after** the GL context is current — that is where `CalendarPage` is constructed and `EstablishConnections` runs. Do not touch GL state before this callback fires.

## Conventions

- C++23, no compiler extensions.
- Header guards use the full-path style `HOME_TITAN99_CODE_DECADE_SRC_<...>_HPP` in most newer files; older files use short guards (e.g. `DECADE_APP_HPP`). Match the surrounding file's style when editing.
- Naming is currently mixed: `*Store` vs. `*Storage`, snake_case members like `signal_page_setup_config` vs. PascalCase `SignalDateGroups()`. When touching a file, prefer the existing local style; do not perform mass renames as a side effect.
- Rule-of-five: classes with explicit destructors should also delete or default copy/move (see `MainWindow`, `DateIntervalBundleStore`).
- Never use raw `new`/`delete`. Always express ownership through smart pointers (`std::unique_ptr` for unique ownership, `std::shared_ptr` for shared ownership) so lifetime is encoded in the type system, exceptions cannot leak resources, and ownership transfer is explicit at call sites. wx widgets are typically transferred via `.release()` to wx-owned parents (wx then owns the lifetime).
- For non-owning references to objects whose lifetime is managed by wxWidgets (wx-owned parents/children, or any class derived from `wxTrackable` — which includes `wxWindow`, `wxEvtHandler`, and most wx classes), prefer `wxWeakRef<T>` over raw pointers. It auto-nulls when the referenced object is destroyed, eliminating dangling-pointer bugs. See [wxWeakRef](https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html) and [wxTrackable](https://docs.wxwidgets.org/3.2/classwx_trackable.html).
