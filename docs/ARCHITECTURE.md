# Decade Architecture

## Goals
- Keep the startup path small and testable.
- Isolate UI wiring from app bootstrap.
- Keep data flow explicit between stores, panels, and renderer.

## Layered Architecture

The codebase follows a four-layer architecture. Dependencies only flow **inward** (Presentation → Application → Domain; Infrastructure is consumed by Application). Domain knows about nothing else; Infrastructure knows about Domain types it serialises.

```
┌────────────────────────────────────────────────────────────┐
│ Presentation        src/gui/ , src/app/main_window.*       │
│   wxWidgets panels, GLCanvas, menus, dialogs               │
├────────────────────────────────────────────────────────────┤
│ Application         src/app/ (incl. src/app/binding/)      │
│   composition root, EventBus, main_window_binder,          │
│   wx app lifecycle, command callbacks                      │
├────────────────────────────────────────────────────────────┤
│ Domain              src/packages/                          │
│   stores, config value types, transformation logic,        │
│   sigslot signals — UI-agnostic                            │
├────────────────────────────────────────────────────────────┤
│ Infrastructure      src/graphics/ , src/app/services/      │
│   OpenGL pipeline, FreeType, XML/CSV/PNG I/O               │
└────────────────────────────────────────────────────────────┘
```

`src/calendar_page.hpp` is the rendering adapter: it bridges Domain state into the Infrastructure scene graph. It belongs conceptually to the Application layer but is kept at the `src/` root for legacy reasons.

### Directory map

- `src/app/`
  - `main.cpp` — process entrypoint only (Application).
  - `decade_app.*` — wx app lifecycle and locale initialisation (Application).
  - `app_config.hpp` — startup/window defaults (Application).
  - `main_window.*` — composition root: owns stores, panels, renderer (Presentation/Application boundary).
  - `services/project_io.*` — XML/CSV/PNG persistence (Infrastructure).
  - `binding/event_bus.hpp` — typed signal hub for domain events (Application).
  - `binding/main_window_binder.hpp` — wires producers/consumers via the EventBus (Application).
- `src/packages/` — Domain stores and config types.
- `src/gui/` — Presentation: wxWidgets panels and the GL canvas wrapper.
- `src/graphics/` — Infrastructure: OpenGL engine, shaders, scene graph, FreeType, PNG export.
- `src/calendar_page.hpp` — rendering adapter (Application/Infrastructure bridge).

## Layering Rules
1. `main.cpp` must not contain business or UI wiring logic.
2. `DecadeApp` may construct high-level objects but should avoid panel/store internals.
3. `MainWindow` owns lifetime; wiring lives in `main_window_binder` (free functions in `src/app/binding/`).
4. `packages/` (Domain) must remain UI-agnostic — no wx, no GL.
5. Domain stores publish state via the `EventBus`; consumers subscribe via the bus rather than connecting to individual store signals.
6. Infrastructure modules take Domain types by reference; they must not depend on Presentation.

## Event flow

Cross-component communication uses an in-process **EventBus** (see `src/app/binding/event_bus.hpp`). The bus owns one typed `sigslot::signal` per domain event. Panels and stores keep their own `Send*` / `Receive*` methods, but the wiring is centralised in `main_window_binder::Bind` — each producer is forwarded into the bus, each consumer subscribes from the bus.

```
Panel ──Send──▶ EventBus.<event> ──▶ Store.Receive
Store ──Send──▶ EventBus.<event> ──▶ Panel.Receive , CalendarPage.Receive , GLCanvas.Receive
```

The `TransformDateIntervalBundle` adapter sits between `date_interval_bundles` (input) and `transformed_date_interval_bundles` (output) on the bus.

When adding new state: create a `*Store` in `packages/`, a panel in `gui/`, add a typed signal to `EventBus`, and register both ends in `main_window_binder::Bind`. `CalendarPage` only needs the `Receive*` slot if the state affects rendering.

## OpenGL initialization

Deferred: `MainWindow` calls `GLCanvas::InitOpenGL(version, callback)`. The callback runs **after** the GL context is current — that is where `CalendarPage` is constructed and `main_window_binder::Bind` runs. Do not touch GL state before this callback fires.

## Follow-up Refactor Targets
1. Add unit tests for CSV/XML conversion logic in `services/project_io`.
2. Consider moving `src/calendar_page.hpp` into `src/app/binding/` and splitting its scene-graph construction into a dedicated builder.
3. Promote stores to publish directly into `EventBus` (removing their internal signals) once all consumers are bus-only.
