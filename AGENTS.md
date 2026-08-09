# decade

Stable guard rails for working on the decade code: architecture and conventions. Build and operations live in [operations.md](operations.md), open points as issues.

## Purpose

A C++26 desktop application for calendars and timelines. Grown out of a prototype; it still carries technical debt (god classes, unclear ownership). The goal is evolutionary refactoring — stepwise, behaviour-preserving, without a rewrite (see [Refactoring](#refactoring)).

Carried by Qt 6 (GUI), OpenGL through libepoxy (rendering), ICU (calendar and locale), Boost.Serialization (XML project files), FreeType, csv2 and Bullet (picking). The full list including submodules stands in `CMakeLists.txt` and `external/` (current state through `git submodule status`). Build, tests, headless runs and the lint gate commands: [operations.md](operations.md).

## Architecture

### Layers and layer rules

The codebase follows a four-layer architecture in the sense of [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html). Dependencies flow **inwards** alone (presentation → application → domain; the application consumes infrastructure). The domain knows nothing else; infrastructure knows only the domain types it serialises. New code must say which layer it belongs to and keep that layer's dependency bounds.

- **Presentation** — the main window (`MainFrame`), Qt panels, the GL canvas (`QOpenGLWidget`), menus, dialogues, file commands. Every panel owns its widgets and offers signals with the same interface as its store. The window owns widgets alone: no stores, no bus, no runtime options.
- **Application** — composition root (`AppComposition`), event bus, binder, project document, startup script, rendering adapter, app lifecycle: it constructs, owns and wires, but carries no domain logic itself.
- **Domain** — value objects, stores, transformation logic, sigslot signals — UI-agnostic and Boost-free (no `friend boost::serialization::access`, no `serialize` members).
- **Infrastructure** — rendering (OpenGL, FreeType), persistence (XML, CSV, PNG), hit testing (Bullet) — non-intrusive, it knows domain types alone.

Rules:

1. The process entry point holds no business or UI wiring logic.
2. The app lifecycle constructs high-level objects but knows no panel or store internals.
3. The composition root owns every lifetime. The wiring lives apart from it in the binder (free functions) — components do not know each other directly. Whatever comes into being only with the GL context (rendering adapter, wiring) sits in an `optional` and gets dissolved when the window is destroyed, while panels and canvas are still alive.
4. The domain stays UI-agnostic — no Qt, no GL, no Boost. The test asks what a library binds to, not its name: a dependency drops out when it ties the domain to an outer layer — a widget toolkit, a graphics context, a file format. That admits glm, which carries the domain colours as `glm::vec4`: header-only maths, no GL header anywhere underneath it, nothing to link, so the domain still builds and runs without a window. The reasoning stands in [#58](https://github.com/schneeregenflocke/decade/issues/58).
5. Stores publish their state themselves on an injected topic; consumers subscribe through the bus. No store owns a signal of its own, and nobody attaches to a store instead of to the bus.
6. Infrastructure takes domain types by reference; it must not depend on presentation.
7. The command line gets read in exactly one place and translated into an options object (`src/application/runtime_options.hpp`); the application reads no environment variables.
8. Serialisation is non-intrusive: it works over public APIs alone and owns the on-disk format; domain types know nothing of persistence.
9. Exactly one bridge connects application and rendering infrastructure: the rendering adapter with its scene composer. Presentation never depends on GL types — it sees the scene graph as a GL-free read model (a snapshot) alone. The way back runs over a port the application declares and presentation implements (`RenderSurface`: the adapter asks for a repaint without knowing the canvas).
10. The application-wide `LocaleDateFormatter` gets constructed once in the composition root and passed on by reference — the locale configuration stays in one place.
11. The open project (every store plus the file path) is one object: `ProjectDocument`. Loading, saving and CSV exchange go over that alone; nobody passes the six stores through one by one.

### Domain pattern: value objects and stores

- **[Value objects](https://martinfowler.com/bliki/ValueObject.html)** hold data plus the queries on it and encapsulate their state: data members `private`, access through const accessors, change through named setters alone — that keeps invariants and on-disk format in one place. No signal, no serialisation, no `friend` → [rule of zero](https://en.cppreference.com/w/cpp/language/rule_of_three), freely copyable.
- **Stores** combine a value object with an injected `domain::StateTopic` and a re-entry guard. They have identity → explicitly not copyable. A store owns no signal of its own: it gets its topic by constructor reference (the port, `domain/state_topic.hpp`) and publishes straight onto it. Who owns the topic is the outer layer's decision — the `EventBus` in the application, a local signal in a test. The topic carries the value (`StateTopic<Value>` = `signal<const Value&>`), so consumers work with copyable value objects; the store offers `Receive`, `Send` and `Get`, no query delegation of its own.
- Value object and store live in **separate files**, named after the main class (`date_group.hpp` plus `date_group_store.hpp`); the value object header has no store dependency.

### Date and interval semantics

- `Date` (a proleptic Gregorian calendar with an explicit invalid state) and `DatePeriod` are the database-free interface. The calendar computations are delegated to ICU and deliberately confined to exactly two places: the internal arithmetic backend and the `LocaleDateFormatter` (language- and locale-dependent parsing and formatting for GUI and CSV). Changing the date library means reimplementing exactly those two places — nobody else reaches for an ICU date API directly.
- Persisted as an [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) string; the earlier format break away from Boost.DateTime sits as history in [#48](https://github.com/schneeregenflocke/decade/issues/48).
- `DatePeriod` is **half-open `[begin, end)` uniformly across the model** (for the reasoning: [Dijkstra, EWD831](https://www.cs.utexas.edu/users/EWD/transcriptions/EWD08xx/EWD831.html)) — `LengthDays()` is `end - begin`, `Last()` the day before `end`, and a period holding no day (`end <= begin`) is *null* (the stores discard null periods). Users think in *inclusive* "from .. to" dates; the conversion happens at exactly two user-facing borders — the date table panel and CSV I/O — through `PeriodFromInclusiveDates()` on the way in and `Last()` on display. Nowhere else should ±1-day arithmetic turn up; keep it that way.

### Event flow (the event bus through sigslot)

Communication across components runs over an in-process **event bus** with one typed topic per domain event. Stores publish straight onto it (see [Domain pattern](#domain-pattern-value-objects-and-stores)); the wiring of the consumers sits centrally in `app_binder::Bind`. You add a new piece of state as a Boost-free value object plus a `*Store` in `domain/`, a panel in `presentation/`, a topic in the `EventBus` and the consumers in the binder; persisted state adds `save`/`load` in `infrastructure/persistence/value_serialization.hpp` and one line in `project_io`.

**Two directions, two rules.** A panel edit is a **command** and goes straight to the owning store — its `Receive*` is the only place canonical state comes into being. The new state is a **fact** and goes over the bus. Were panels to put their edits onto the same topic they subscribe to, there would be feedback loops. Only where a producer sits in presentation and therefore gets no topic injected (font choice, tree selection) does `detail::Forward` hang the panel signal onto the topic. The scene tree is the counter-check: it mirrors the render graph and only displays — every row of its detail grid is read-only, because an edit field there would be a second write path onto state the rebuild recreates anyway. The shape configurations get edited where they live, in the Shapes panel.

One topic carries no state but a bracket around it: `state_burst` opens with `true` and closes with `false`, held by the RAII `StateBurst`. One user action often fills several stores in a row — loading a project fills six — and a consumer that rebuilds per change would do the work six times. It sits on the bus rather than behind a port of its own, because the producer (`ProjectDocument`) exists long before the consumer (the rendering adapter, which waits for the GL context): over the bus, whoever is not there simply does not listen. Ignoring it stays correct — the bracket is an offer, not a protocol.

The wiring itself is a lifetime, not a pair of calls: `AppWiring` connects on construction and disconnects on destruction. In the composition root it stands as the last member and thereby dies before stores and bus.

**Editing in the canvas** follows the same rule, only time-shifted: the editor (`TitleTextEditor`) holds a buffer and publishes a GL-free read model (`TextEditView`) on every keystroke, out of which the renderer draws text, cursor and selection. The text becomes canonical with Enter alone — then it goes to the store as *one* command; Esc discards the buffer. That way an edit is exactly one state change, not one per key. The key codes stay in presentation: the canvas translates them into a `TextInputEvent`, and the editor knows its meaning alone.

## Decisions

### Language standard and header guards

- C++26, no compiler extensions. The move up from C++23 was made for [`#embed`](https://en.cppreference.com/cpp/preprocessor/embed), which carries the shaders and licence texts into the binary and replaced a submodule with a generator binary ([#79](https://github.com/schneeregenflocke/decade/issues/79)). Before C++26 both GCC and Clang grade it as an extension and `-Wpedantic -Werror` rejects it, so the standard level is not cosmetic here.
- Header guards use the file name style: the upper-cased file name with the dot before the suffix as `_`, for instance `main_window.hpp` → `MAIN_WINDOW_HPP`, `gl_canvas.hpp` → `GL_CANVAS_HPP`. No directory path prefix. Apply that consistently in `#ifndef`, `#define` and the closing `#endif  // <GUARD>` comment. (The clang-tidy check `llvm-header-guard`, which would otherwise force a full-path style, is switched off in `.clang-tidy` — leave it that way.)

### Refactoring

The goal is self-documenting code; refactoring brings it there step by step.

- Safety before structure. Before a structural change to a god class or another [code smell](https://en.wikipedia.org/wiki/Code_smell), add a black-box safety net first: a [characterisation test](https://en.wikipedia.org/wiki/Characterization_test) with an input and the current output as a frozen expectation. No rebuild without that cover.
- Do not change behaviour while tidying. Formatting, renames, warning cleanup and behaviour changes are separate steps or commits. When fixing a warning, never change the semantics in silence.
- Small, reversible steps. One commit, one goal. Keep diffs small enough to roll back cleanly. For bigger rebuilds: the Mikado method (Ellnestam and Brolund) — note the goal, explore the preconditions, roll back on a break instead of pushing through.
- Refactoring is evolutionary, not a rewrite. First lower the risk, then cut along the load-bearing abstraction.
- Isolate pure formatting commits and enter them in `.git-blame-ignore-revs` ([git blame --ignore-revs-file](https://git-scm.com/docs/git-blame)), so the history stays readable.
- The stepwise order: first stabilise (characterisation tests, smoke paths, a baseline output) → then split (extract small seams after Michael Feathers, "Working Effectively with Legacy Code", behaviour unchanged) → then rename (make the intent visible without changing semantics) → last decouple (remove coupling only once the form is already safe).
- Whatever cannot be changed at once becomes an issue, so it does not get lost.
- Read the whole file, not just the task. A misleading name, a duplicated block, a violated convention: fix it right away as its **own** commit, or open an issue when the fix outgrows the task or needs a decision. Noticing without acting is no option.

### Style

Language and documentation rules live in the superproject (`~/homelab-superproject/AGENTS.md`).

### Ownership and lifetimes

- Never use raw `new`/`delete`. Always express ownership through a smart pointer, so the lifetime is encoded in the type system, exceptions leak no resources and ownership handovers are explicit at call sites (compare the C++ Core Guidelines, [resource management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-resource)). Qt widgets typically get handed to their parent through `.release()` (the parent then owns the lifetime).
- The order of choice: value semantics, stack or container first → `std::unique_ptr` for exclusive ownership → `std::shared_ptr` *only* where ownership is demonstrably shared (a design signal, not a default) → `std::weak_ptr` against cycles.
- [Raw pointers are never owners](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr); as non-owning observers they are admissible in parameters and locals alone. **Raw pointer data members are forbidden** — non-owning ones too: a raw `T*` member encodes neither ownership nor lifetime and is the classic dangling pointer trap. Express the relation in the type system instead:
  - **A non-owning reference to a `QObject`** (every widget, layout and `QAction`): use [`QPointer<T>`](https://doc.qt.io/qt-6/qpointer.html). It resets itself to null when the referenced object gets destroyed.
  - **A non-owning reference to a Qt-owned object that is no `QObject`** (`QTreeWidgetItem` and `QTableWidgetItem` are the ones here, so `QPointer` does not compile): **cache no pointer at all**. Fetch the object from its owner when needed — out of the signal carrying it, or by looking it up under a stable key the creation and the lookup share (`SceneTreePanel` stores the node path in the item and searches by it). The temporary `T*` a Qt API returns at the call site is fine, and so is one travelling as a parameter; storing it as a member is not.
  - **Owning a heap object**: a smart pointer (`std::unique_ptr` or `std::shared_ptr`), or `MakeOwned<T>` when a Qt object gets handed to its parent.
- [Rule of five](https://en.cppreference.com/w/cpp/language/rule_of_three): classes with explicit destructors should delete or default copy and move as well (see `MainWindow`, `DateEntryStore`).

### C++ discipline

- [const-correctness](https://isocpp.org/wiki/faq/const-correctness) throughout: SSOT for mutability — whatever does not get changed is `const`.
- Include discipline: dissolve include thickets (forward declarations, minimal includes, [include-what-you-use](https://include-what-you-use.org/)). A bigger structural lever than reformatting, and it lowers compile times along the way.
- A contiguous sequence travels as a [`std::span`](https://en.cppreference.com/w/cpp/container/span), never as a pointer beside a count — two parameters that can disagree at a call site are one parameter too many.
- Where null is no valid value, say so with a **reference**, not with an annotation. That is the stronger statement and needs no library. How much further the hardening goes — `gsl::not_null`, `span` on the read-only container parameters — is still open in [#42](https://github.com/schneeregenflocke/decade/issues/42); until it closes, add no blanket annotation. A sweep would also be the wrong first move: the hole is in the raw pointer *members* the ban above already forbids and that a few places still carry, and no annotation on a parameter reaches those.

### Components: a header and its translation unit

Every header pairs with a translation unit of the same name — the **component** after John Lakos ("Large-Scale C++ Software Design"). The header declares, the `.cpp` defines. A member gets **declared** in the class body and **defined** out of line in the component's `.cpp`: the body then reads as an interface, and editing a definition rebuilds one unit instead of the program. The move off the earlier header-only design, with the measurements that carried the decision, runs in [#88](https://github.com/schneeregenflocke/decade/issues/88).

Whatever the language holds visible stays in the header and needs no argument: templates, `constexpr` and `consteval` constants, and anything a consumer must instantiate itself. Everything else needs a reason at the place itself to stay there. A function that remains in the header at namespace scope carries `inline` itself, against ODR violations once a second unit includes it. Cutting one header into smaller headers stays welcome.

`AUTOMOC` is **on**, and a presentation class may carry `Q_OBJECT`: with a translation unit per header, the unit [moc](https://doc.qt.io/qt-6/moc.html) emits costs nothing the design forbids. That alone is no reason to reach for it — the application's own signals run over sigslot, and subscribing to a Qt widget's signal works through `QObject::connect` with a functor without any of it. Whether the bus moves to Qt signals stands open in [#86](https://github.com/schneeregenflocke/decade/issues/86) and [#74](https://github.com/schneeregenflocke/decade/issues/74); until that closes, domain and application keep their Qt-free wiring (layer rule 4).

### OpenGL under Qt

Three traps, all silent, each settled in one place.

**libepoxy has to be first.** `epoxy/gl.h` claims the include guards `__gl_h_` and `__glext_h_` and refuses to compile once `GL/gl.h` got there before it (an `#error`). Qt's `qopengl.h` — which `QOpenGLWidget` pulls in — includes exactly those, so a header that reaches Qt first breaks the build. Source order cannot settle it: clang-format sorts `<QtOpenGLWidgets/…>` ahead of `<epoxy/gl.h>` and Google style regroups include blocks anyway. Hence `-include epoxy/gl.h` as a target compile option in `CMakeLists.txt`, once for everything. With the guards taken, Qt's GL headers collapse to nothing and every `gl*` call resolves through epoxy's dispatch — [which is what Qt's "avoid calling the functions directly" is actually about](https://doc.qt.io/qt-6/opengl-changes-qt6.html): it warns against link-time symbols against a linked GL library, and epoxy is itself a loader.

**A context is current in three callbacks alone.** `QOpenGLWidget` makes it current for `initializeGL`, `resizeGL` and `paintGL`, and it renders into a framebuffer object of its own — so framebuffer 0 is not the screen, and `glGetIntegerv(GL_VIEWPORT)` in a mouse handler asks nothing. Three rules follow, and each has a place that keeps it: whoever builds or destroys GL objects outside those callbacks brackets it with `GLCanvas::MakeGraphicsCurrent` (never with a `doneCurrent` — that would pull the context out from under Qt when it runs nested); whoever binds a framebuffer restores the one that was bound rather than 0 (`ScopedFramebufferBinding`); and whoever needs the viewport takes it as a parameter (`MouseInteraction`). A platform that carries no GL widget at all — the `offscreen` plugin — reports through the failure path instead of dying at the first call.

**Alpha has to leave the framebuffer at 1.** The canvas draws an opaque page, but `GL_SRC_ALPHA` blends the alpha channel with itself, so a translucent fill leaves a value below 1 behind — colour that is already final beside an alpha that means nothing. The buffer does not stay inside: `QOpenGLWidget` hands it to the compositor, and the export writes it into the PNG. Whoever reads it as premultiplied divides the colour by that alpha and wraps at 8 bit, which is how teal came out red ([#93](https://github.com/schneeregenflocke/decade/issues/93)). Hence `glBlendFuncSeparate` with `GL_ONE` on the alpha channel in `GLCanvas::ApplyInitialGlState`: colour blends as before, alpha saturates. Retagging a read-back afterwards would patch one consumer and miss the rest.

### Warnings, the clang-tidy and the sanitizer gate

- **Warnings break the build — fix them, do not suppress them. That rule holds for compiler warnings and for [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) diagnostics alike.** Never quieten a finding with [`NOLINT`, `NOLINTNEXTLINE` or `NOLINTBEGIN`](https://clang.llvm.org/extra/clang-tidy/index.html#suppressing-undesired-diagnostics), a `#pragma`, a `-Wno-…` flag or a single-line exception in `.clang-tidy`. Change the code so the finding no longer takes hold. Restructuring, RAII and correct type annotations are fixes; suppression is not.
  - **Prefer a real fix, even where the warning looks "unfixable" at first.** Usually it is not. An example: [`cppcoreguidelines-owning-memory`](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/owning-memory.html) over a raw C resource is satisfied once ownership is expressed with `gsl::owner<>` from the [GSL](https://github.com/microsoft/GSL) — the project already links `Microsoft.GSL::GSL` — and a `std::unique_ptr` with a custom deleter; see `src/infrastructure/graphics/png_writer.hpp`. A rule that truly does not apply to the project style gets switched off **once, globally** in `.clang-tidy` with a comment (as `-modernize-use-trailing-return-type` and `-llvm-header-guard` already are) — never spread per line.
  - **Suppression is a last resort alone, and only for constructs a third-party C API forces on us contractually and that the code cannot solve otherwise.** The canonical (and currently only admitted) example is libpng's mandatory `setjmp`/`longjmp` error handling in `src/infrastructure/graphics/png_writer.hpp`. If you must suppress: scope the `NOLINT` to the **concrete check names** (never a bare `NOLINT`), confine it to the narrowest line and add a comment explaining *why* it is unfixable. Where a whole dependency makes a class of warning unavoidable, replace that dependency rather than spread suppressions.
  - The one `-Wno-…` in the build is **not** an exception to the rule but a statement about a compiler: `-Wno-c23-extensions` says that Clang 22 has not yet implemented `#embed` as the C++26 feature it is. It says nothing about our code — GCC compiles the same line as standard under `-std=c++26 -Wpedantic -Werror`. It stands in the clang branch of `CMakeLists.txt` and goes the day Clang catches up.

One build directory, one compiler in it: `build/` holds GCC or clang, chosen when you configure, and the choice decides which targets exist — `clang-tidy` and `sanitize-memory` appear under clang alone, because they parse with its frontend. `CMakeLists.txt` marks the difference through `DECADE_GCC` and `DECADE_CLANG`; whatever holds for both carries no guard. CI runs one job per compiler, and both build and test, because each sees warnings the other does not. Whoever adds a gate says which compiler it belongs to.

Beside clang-tidy stands the sanitizer gate `sanitize-address` (address, leak, undefined): it rebuilds the tree instrumented and runs the test suite underneath. It works under both compilers and gets held at **zero findings**; a sanitizer hit gets fixed, not suppressed. Whoever changes behaviour the tests do not cover, covers it first — the sanitizer sees what runs alone.

The gate commands (enforcement targets, the full run, auto-fix, clang-format, CI) stand in [operations.md](operations.md), section Build checks.

### Principles

Binding design principles. The established terms are set here — as everywhere in this document — deliberately as [semantic anchors](https://github.com/LLM-Coding/Semantic-Anchors): the term activates the knowledge behind it, in humans as in coding agents, more precisely than any paraphrase. As a general C++ guideline the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#main) hold throughout, and many of the anchors below come from them.

- Check against the official manual, not from memory: when working with Qt, OpenGL, ICU, Boost, clang-tidy or CMake, read the documentation of the **version used here** — behaviour, flags and defaults change between versions.
- [Single responsibility principle](https://en.wikipedia.org/wiki/Single-responsibility_principle) and [separation of concerns](https://en.wikipedia.org/wiki/Separation_of_concerns); low [coupling](https://en.wikipedia.org/wiki/Coupling_%28computer_programming%29), high [cohesion](https://en.wikipedia.org/wiki/Cohesion_%28computer_science%29).
- [Domain-driven design](https://www.domainlanguage.com/ddd/reference/) — see the [domain pattern](#domain-pattern-value-objects-and-stores) — and [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) — see [Layers and layer rules](#layers-and-layer-rules).
- [DRY](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself) as DRY of knowledge, with [single source of truth](https://en.wikipedia.org/wiki/Single_source_of_truth) as the measure: every piece of knowledge (a rule, a constant, a domain decision) has exactly one authoritative representation — not every similar-looking line folded together. But: [duplication is cheaper than the wrong abstraction](https://sandimetz.com/blog/2016/1/20/the-wrong-abstraction); two coincidentally identical blocks expressing *different* concepts stay apart — in doubt, do not abstract early ([YAGNI](https://en.wikipedia.org/wiki/You_aren%27t_gonna_need_it), [KISS](https://en.wikipedia.org/wiki/KISS_principle)).
  - The mechanics: where the same multi-line shape recurs across several methods (or panels), pull it up into a small helper — a `private` member, a free function or a shared base class — instead of copying it. Established examples: `scene_shapes::FillRectangles` and `AddCenteredText` (scene node creation), `runtime_options_detail::FoundString` (read an option → `std::optional<std::string>`), `MakeOwned<T>` (parent-owned widgets), `TablePanelBase` (the table plus add and delete scaffold), `serialization_detail::ColorToArray` and `ColorFromArray` (glm::vec4 marshalling). Prefer that over macros, because macros worsen readability and debuggability — the explicit, field-by-field `save`/`load` pairs in `infrastructure/persistence/value_serialization.hpp` stay written out on purpose, because they document the on-disk format.
- [Principle of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment): names, signatures and behaviour fit together.
- Choose the smallest useful abstraction; prefer explicit data flow over hidden coupling ([law of Demeter](https://en.wikipedia.org/wiki/Law_of_Demeter)); encapsulate unwieldy constructs instead of spreading them.
- [GRASP](https://en.wikipedia.org/wiki/GRASP_%28object-oriented_design%29) heuristics for assigning responsibility when deciding where code belongs: information expert, creator, controller, low coupling and high cohesion, indirection, pure fabrication, polymorphism and protected variations.
- Keep stable rules apart from unstable work (this file against the issues).

### Self-documenting code

The code communicates its intent itself; prose is the exception. The guard rail is P.1 "[Express ideas directly in code](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rp-direct)" of the C++ Core Guidelines. What the code or a command already shows does not get documented on top.

- **Names carry the purpose, not the mechanism** — on every level: variables, functions, classes, members. Anchors: intention-revealing selector (Kent Beck, "Smalltalk Best Practice Patterns") for names; intention-revealing interfaces (Eric Evans, [DDD Reference](https://www.domainlanguage.com/ddd/reference/)) for interfaces; [general naming rules](https://google.github.io/styleguide/cppguide.html#General_Naming_Rules): optimise for readability, no cryptic abbreviations.
- **Structure explains itself:** small units with one responsibility; one level of abstraction per function (SLAP); deep modules — a small interface with much functionality behind it (John Ousterhout, "[A Philosophy of Software Design](https://web.stanford.edu/~ouster/cgi-bin/book.php)").
- **Comments are sparing** and explain the non-obvious why alone (a decision, a trade-off), never the what ([NL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-naming) of the Core Guidelines). A comment describing *what* the code does is a hint to make the code clearer — not to keep the comment.

Concretely, after [Google C++ Style](https://google.github.io/styleguide/cppguide.html#Naming) (in force):

- Types: `PascalCase` (`DateGroup`).
- Functions and methods: `PascalCase` (`GetDateGroups()`); trivial accessors and mutators may carry `snake_case` like their member (`set_count()`).
- Class data members: `snake_case` **with a trailing underscore** (`date_format_`). Struct members without one. The clang-tidy gate enforces this member rule ([readability-identifier-naming](https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html) in `.clang-tidy`); a member without an underscore breaks the build.
- Locals: `snake_case`. Constants and enumerators: `kPascalCase` (`kColorScale`).
- The store suffix is uniformly `…Store` (not `…Storage`) — for types **and** for member and parameter names (`…_store`, not `…_storage`).
- Renames that unify spelling and identifiers are welcome. When renaming, do it **completely and consistently** across every occurrence (declaration, definition, call sites, tests, documentation) — no half rename leaving two spellings side by side. Keep the build green afterwards (compile plus `ctest` plus the clang-tidy gate).
