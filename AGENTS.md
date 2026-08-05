# decade

Stable guard rails for working on the decade code: architecture and conventions. Build and operations live in [operations.md](operations.md), open points as issues.

## Purpose

A C++23 desktop application for calendars and timelines. Grown out of a prototype; it still carries technical debt (god classes, unclear ownership). The goal is evolutionary refactoring — stepwise, behaviour-preserving, without a rewrite (see [Refactoring](#refactoring)).

Carried by wxWidgets (GUI), OpenGL through libepoxy (rendering), ICU (calendar and locale), Boost.Serialization (XML project files), FreeType, csv2 and Bullet (picking). The full list including submodules stands in `CMakeLists.txt` and `external/` (current state through `git submodule status`). Build, tests, headless runs and the lint gate commands: [operations.md](operations.md).

## Architecture

### Goals

- Keep the startup path small and testable.
- Isolate UI wiring from the app bootstrap.
- Keep the data flow between stores, panels and renderer explicit.
- Clear layering and self-explaining names, so the code stays navigable — see [Self-documenting code](#self-documenting-code).

### Layers and layer rules

The codebase follows a four-layer architecture in the sense of [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html). Dependencies flow **inwards** alone (presentation → application → domain; the application consumes infrastructure). The domain knows nothing else; infrastructure knows only the domain types it serialises. New code must say which layer it belongs to and keep that layer's dependency bounds.

- **Presentation** — the main window (`MainFrame`), wxWidgets panels, the GL canvas wrapper, menus, dialogues, file commands. Every panel owns its widgets and offers signals with the same interface as its store. The window owns widgets alone: no stores, no bus, no runtime options.
- **Application** — composition root (`AppComposition`), event bus, binder, project document, startup script, rendering adapter, app lifecycle: it constructs, owns and wires, but carries no domain logic itself.
- **Domain** — value objects, stores, transformation logic, sigslot signals — UI-agnostic and Boost-free (no `friend boost::serialization::access`, no `serialize` members).
- **Infrastructure** — rendering (OpenGL, FreeType), persistence (XML, CSV, PNG), hit testing (Bullet) — non-intrusive, it knows domain types alone.

Rules:

1. The process entry point holds no business or UI wiring logic.
2. The app lifecycle constructs high-level objects but knows no panel or store internals.
3. The composition root owns every lifetime. The wiring lives apart from it in the binder (free functions) — components do not know each other directly. Whatever comes into being only with the GL context (rendering adapter, wiring) sits in an `optional` and gets dissolved when the window is destroyed, while panels and canvas are still alive.
4. The domain stays UI-agnostic — no wx, no GL, no Boost.
5. Stores publish their state themselves on an injected topic; consumers subscribe through the bus. No store owns a signal of its own, and nobody attaches to a store instead of to the bus.
6. Infrastructure takes domain types by reference; it must not depend on presentation.
7. The command line gets read in exactly one place and translated into an options object (`src/application/runtime_options.hpp`); the application reads no environment variables.
8. Serialisation is non-intrusive: it works over public APIs alone and owns the on-disk format; domain types know nothing of persistence.
9. Exactly one bridge connects application and rendering infrastructure: the rendering adapter with its scene composer. Presentation never depends on GL types — it sees the scene graph as a GL-free read model (a snapshot) alone.
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

**Two directions, two rules.** A panel edit is a **command** and goes straight to the owning store — its `Receive*` is the only place canonical state comes into being. The new state is a **fact** and goes over the bus. Were panels to put their edits onto the same topic they subscribe to, there would be feedback loops. Only where a producer sits in presentation and therefore gets no topic injected (font choice, tree selection) does `detail::Forward` hang the panel signal onto the topic. The scene tree is the counter-check: it mirrors the render graph and only displays — every row of its detail grid is read-only, because an edit field there would be a second write path onto state the rebuild recreates anyway.

The wiring itself is a lifetime, not a pair of calls: `AppWiring` connects on construction and disconnects on destruction. In the composition root it stands as the last member and thereby dies before stores and bus.

**Editing in the canvas** follows the same rule, only time-shifted: the editor (`TitleTextEditor`) holds a buffer and publishes a GL-free read model (`TextEditView`) on every keystroke, out of which the renderer draws text, cursor and selection. The text becomes canonical with Enter alone — then it goes to the store as *one* command; Esc discards the buffer. That way an edit is exactly one state change, not one per key. The key codes stay in presentation: the canvas translates them into a `TextInputEvent`, and the editor knows its meaning alone.

## Decisions

### Language standard and header guards

- C++23, no compiler extensions.
- Header guards use the file name style: the upper-cased file name with the dot before the suffix as `_`, for instance `main_window.hpp` → `MAIN_WINDOW_HPP`, `gl_canvas.hpp` → `GL_CANVAS_HPP`. No directory path prefix. Apply that consistently in `#ifndef`, `#define` and the closing `#endif  // <GUARD>` comment. (The clang-tidy check `llvm-header-guard`, which would otherwise force a full-path style, is switched off in `.clang-tidy` — leave it that way.)

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

### Refactoring

The goal is self-documenting code; refactoring brings it there step by step.

- Safety before structure. Before a structural change to a god class or another [code smell](https://en.wikipedia.org/wiki/Code_smell), add a black-box safety net first: a [characterisation test](https://en.wikipedia.org/wiki/Characterization_test) with an input and the current output as a frozen expectation. No rebuild without that cover.
- Do not change behaviour while tidying. Formatting, renames, warning cleanup and behaviour changes are separate steps or commits. When fixing a warning, never change the semantics in silence.
- Small, reversible steps. One commit, one goal. Keep diffs small enough to roll back cleanly. For bigger rebuilds: the Mikado method (Ellnestam and Brolund) — note the goal, explore the preconditions, roll back on a break instead of pushing through.
- Refactoring is evolutionary, not a rewrite. First lower the risk, then cut along the load-bearing abstraction.
- Isolate pure formatting commits and enter them in `.git-blame-ignore-revs` ([git blame --ignore-revs-file](https://git-scm.com/docs/git-blame)), so the history stays readable.
- The stepwise order: first stabilise (characterisation tests, smoke paths, a baseline output) → then split (extract small seams after Michael Feathers, "Working Effectively with Legacy Code", behaviour unchanged) → then rename (make the intent visible without changing semantics) → last decouple (remove coupling only once the form is already safe).
- Whatever cannot be changed at once becomes an issue, so it does not get lost.
- Violations of this file's conventions that you notice while working on a file become an issue too — even when they are no part of the task.

### Style

Language and documentation rules live in the superproject (`~/homelab-superproject/AGENTS.md`).

### Ownership and lifetimes

- Never use raw `new`/`delete`. Always express ownership through a smart pointer, so the lifetime is encoded in the type system, exceptions leak no resources and ownership handovers are explicit at call sites (compare the C++ Core Guidelines, [resource management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-resource)). wx widgets typically get handed to wx-owned parents through `.release()` (wx then owns the lifetime).
- The order of choice: value semantics, stack or container first → `std::unique_ptr` for exclusive ownership → `std::shared_ptr` *only* where ownership is demonstrably shared (a design signal, not a default) → `std::weak_ptr` against cycles.
- [Raw pointers are never owners](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr); as non-owning observers they are admissible in parameters and locals alone. **Raw pointer data members are forbidden** — non-owning ones too: a raw `T*` member encodes neither ownership nor lifetime and is the classic dangling pointer trap. Express the relation in the type system instead:
  - **A non-owning reference to an object derived from `wxTrackable`** (wx-owned parents and children — `wxWindow`, `wxEvtHandler` and most wx classes): use [`wxWeakRef<T>`](https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html). It resets itself to null when the referenced object gets destroyed (see [wxTrackable](https://docs.wxwidgets.org/3.2/classwx_trackable.html)).
  - **A non-owning reference to a wx-owned object that is *not* `wxTrackable`** (`wxPGProperty` inherits from `wxObject`, for instance, so `wxWeakRef` does not compile): **cache no pointer at all**. Fetch the object from its owner when needed — out of the event carrying it (`wxPropertyGridEvent::GetProperty()`), or by name lookup at the owner (`wxPropertyGrid::GetPropertyByName()`) — with a stable name constant that creation and lookup share. The temporary `T*` a wx API returns at the call site is fine; storing it as a member is not.
  - **Owning a heap object**: a smart pointer (`std::unique_ptr` or `std::shared_ptr`), or `MakeOwned<T>` when a wx widget gets handed to its wx-owned parent.
- [Rule of five](https://en.cppreference.com/w/cpp/language/rule_of_three): classes with explicit destructors should delete or default copy and move as well (see `MainWindow`, `DateEntryStore`).

### C++ discipline

- [const-correctness](https://isocpp.org/wiki/faq/const-correctness) throughout: SSOT for mutability — whatever does not get changed is `const`.
- Include discipline: dissolve include thickets (forward declarations, minimal includes, [include-what-you-use](https://include-what-you-use.org/)). A bigger structural lever than reformatting, and it lowers compile times along the way.

### Designed as header-only

The codebase is **deliberately designed as header-only** (`main.cpp` is the single translation unit). When adding code, extend an existing header rather than splitting into a `.cpp`. Keep that convention through refactorings too. Definitions living in a header must be [`inline`](https://en.cppreference.com/w/cpp/language/inline) (free functions and out-of-class member definitions), so the single-TU rule does not quietly hide ODR violations should a header ever get included from elsewhere (tests, for instance).

### Warnings, the clang-tidy and the sanitizer gate

- **Warnings break the build — fix them, do not suppress them. That rule holds for compiler warnings and for [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) diagnostics alike.** Never quieten a finding with [`NOLINT`, `NOLINTNEXTLINE` or `NOLINTBEGIN`](https://clang.llvm.org/extra/clang-tidy/index.html#suppressing-undesired-diagnostics), a `#pragma`, a `-Wno-…` flag or a single-line exception in `.clang-tidy`. Change the code so the finding no longer takes hold. Restructuring, RAII and correct type annotations are fixes; suppression is not.
  - **Prefer a real fix, even where the warning looks "unfixable" at first.** Usually it is not. An example: [`cppcoreguidelines-owning-memory`](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/owning-memory.html) over a raw C resource is satisfied once ownership is expressed with `gsl::owner<>` from the [GSL](https://github.com/microsoft/GSL) — the project already links `Microsoft.GSL::GSL` — and a `std::unique_ptr` with a custom deleter; see `src/infrastructure/graphics/png_writer.hpp`. A rule that truly does not apply to the project style gets switched off **once, globally** in `.clang-tidy` with a comment (as `-modernize-use-trailing-return-type` and `-llvm-header-guard` already are) — never spread per line.
  - **Suppression is a last resort alone, and only for constructs a third-party C API forces on us contractually and that the code cannot solve otherwise.** The canonical (and currently only admitted) example is libpng's mandatory `setjmp`/`longjmp` error handling in `src/infrastructure/graphics/png_writer.hpp`. If you must suppress: scope the `NOLINT` to the **concrete check names** (never a bare `NOLINT`), confine it to the narrowest line and add a comment explaining *why* it is unfixable. Where a whole dependency makes a class of warning unavoidable, replace that dependency rather than spread suppressions.

Beside clang-tidy stands the sanitizer gate `sanitize-address` (address, leak, undefined): it rebuilds the tree instrumented and runs the test suite underneath. That tree too gets held at **zero findings**; a sanitizer hit gets fixed, not suppressed. Whoever changes behaviour the tests do not cover, covers it first — the sanitizer sees what runs alone.

The gate commands (enforcement targets, the full run, auto-fix, clang-format, CI) stand in [operations.md](operations.md), section Build checks.

### Principles

Binding design principles. The established terms are set here — as everywhere in this document — deliberately as [semantic anchors](https://github.com/LLM-Coding/Semantic-Anchors): the term activates the knowledge behind it, in humans as in coding agents, more precisely than any paraphrase. As a general C++ guideline the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#main) hold throughout, and many of the anchors below come from them.

- Check against the official manual, not from memory: when working with wxWidgets, OpenGL, ICU, Boost, clang-tidy or CMake, read the documentation of the **version used here** — behaviour, flags and defaults change between versions.
- [Single responsibility principle](https://en.wikipedia.org/wiki/Single-responsibility_principle) and [separation of concerns](https://en.wikipedia.org/wiki/Separation_of_concerns); low [coupling](https://en.wikipedia.org/wiki/Coupling_%28computer_programming%29), high [cohesion](https://en.wikipedia.org/wiki/Cohesion_%28computer_science%29).
- [Domain-driven design](https://www.domainlanguage.com/ddd/reference/) — see the [domain pattern](#domain-pattern-value-objects-and-stores) — and [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) — see [Layers and layer rules](#layers-and-layer-rules).
- [DRY](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself) as DRY of knowledge, with [single source of truth](https://en.wikipedia.org/wiki/Single_source_of_truth) as the measure: every piece of knowledge (a rule, a constant, a domain decision) has exactly one authoritative representation — not every similar-looking line folded together. But: [duplication is cheaper than the wrong abstraction](https://sandimetz.com/blog/2016/1/20/the-wrong-abstraction); two coincidentally identical blocks expressing *different* concepts stay apart — in doubt, do not abstract early ([YAGNI](https://en.wikipedia.org/wiki/You_aren%27t_gonna_need_it), [KISS](https://en.wikipedia.org/wiki/KISS_principle)).
  - The mechanics: where the same multi-line shape recurs across several methods (or panels), pull it up into a small helper — a `private` member, a free function or a shared base class — instead of copying it. Established examples: `scene_shapes::FillRectangles` and `AddCenteredText` (scene node creation), `runtime_options_detail::FoundString` (read an option → `std::optional<std::string>`), `MakeOwned<T>` (parent-owned widgets), `TablePanelBase` (the table plus add and delete scaffold), `serialization_detail::ColorToArray` and `ColorFromArray` (glm::vec4 marshalling). Prefer that over macros, because macros worsen readability and debuggability — the explicit, field-by-field `save`/`load` pairs in `infrastructure/persistence/value_serialization.hpp` stay written out on purpose, because they document the on-disk format.
- [Principle of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment): names, signatures and behaviour fit together.
- Choose the smallest useful abstraction; prefer explicit data flow over hidden coupling ([law of Demeter](https://en.wikipedia.org/wiki/Law_of_Demeter)); encapsulate unwieldy constructs instead of spreading them.
- [GRASP](https://en.wikipedia.org/wiki/GRASP_%28object-oriented_design%29) heuristics for assigning responsibility when deciding where code belongs: information expert, creator, controller, low coupling and high cohesion, indirection, pure fabrication, polymorphism and protected variations.
- Keep stable rules apart from unstable work (this file against the issues).
