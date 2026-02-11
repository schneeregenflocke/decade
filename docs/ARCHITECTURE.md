# Decade Architecture

## Goals
- Keep the startup path small and testable.
- Isolate UI wiring from app bootstrap.
- Keep data flow explicit between stores, panels, and renderer.

## Current Structure
- `src/app`
  - `main.cpp`: process entrypoint only.
  - `decade_app.*`: wx app lifecycle and locale initialization.
  - `app_config.hpp`: startup/window defaults.
  - `main_window.*`: composition root for UI, services, and event wiring.
- `src/packages`
  - Domain/config stores and transformation logic.
- `src/gui`
  - wxWidgets panels and controls.
- `src/graphics`, `src/calendar_page.hpp`
  - OpenGL canvas and rendering.

## Layering Rules
1. `main.cpp` must not contain business or UI wiring logic.
2. `DecadeApp` may construct high-level objects but should avoid panel/store internals.
3. `MainWindow` owns wiring between panels, stores, and renderer.
4. `packages` must remain UI-agnostic.

## Follow-up Refactor Targets
1. Extract file import/export (XML/CSV/PNG) into dedicated services under `src/app/services`.
2. Move signal wiring from `MainWindow::EstablishConnections` into a dedicated `MainWindowBinder`.
3. Add unit tests for CSV/XML conversion logic once services are extracted.
