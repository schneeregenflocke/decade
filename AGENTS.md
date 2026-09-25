# decade

Stable guard rails for working on the decade code: architecture and conventions. Build and operations live in [operations.md](operations.md), open points as issues.

## Principles

### How these rules hold

- **Anchors, not mandates.** The established terms stand here as [semantic anchors](https://github.com/LLM-Coding/Semantic-Anchors): a term calls up the knowledge behind it, in humans as in coding agents, more precisely than any paraphrase — without prescribing each step.
- **Tools enforce what they can check.** clang-tidy, the tests and the sanitizer gate carry every rule a tool can decide, so nobody has to keep it in mind.
- **Examples in the code show instead of demand**, such as the established helpers under [Design principles](#design-principles).
- **Tool-driven moves** — a rename through clangd, a clang-tidy fix-it — preserve behaviour by construction.
- **A concrete refactoring target becomes an issue**, where the case gets a goal of its own instead of a general rule narrowing it.

### Refactoring

The target is self-documenting code (see [Self-documenting code](#self-documenting-code)), reached in behaviour-preserving steps. Martin Fowler's [refactoring catalogue](https://refactoring.com/catalog/) gives the moves their names — Rename Variable, Extract Function, Move Function — a vocabulary to reach for when a name helps, never a demand.

- **Two hats.** A commit either restructures or changes behaviour, never both (Kent Beck). A restructuring leaves every test result and every output unchanged; a warning fix counts as one and never changes the semantics in silence.
- **Preparatory refactoring.** Make the change easy, then make the easy change ([Kent Beck, via Fowler](https://martinfowler.com/articles/preparatory-refactoring-example.html)): before a feature, restructure until it fits, in a commit of its own.
- **Characterisation test first.** Before restructuring code no test covers, pin what it does today in a test (Michael Feathers, "Working Effectively with Legacy Code"). The sanitizer gate sees only what runs, so uncovered code stays unchecked by it too.
- **Too large for one step: the [Mikado Method](https://www.manning.com/books/the-mikado-method)** (Ola Ellnestam, Daniel Brolund). Attempt the goal, note what breaks, revert, and do the prerequisites first; the graph of prerequisites becomes the plan, kept in the issue.
- **Replacing a component: the [strangler fig](https://martinfowler.com/bliki/StranglerFigApplication.html)** (Fowler). The new one grows beside the old, the callers move over one by one, and the old one goes once nobody calls it.
- **A rename goes all the way.** Renames that unify spelling are welcome, and one covers every occurrence — declaration, definition, call sites, tests, comments, documentation, open issues — and the names derived from it: a config struct, a member or parameter holding the object, a constant, a command-line option. It ends when a search for the old spelling finds history alone.
- **Green means every gate.** Compile, `ctest`, the clang-tidy gate and `sanitize-address` (commands in [operations.md](operations.md)).
- **Read the whole file, not just the task.** A misleading name, a duplicated block, a violated convention: fix it right away as its **own** commit, or open an issue when the fix outgrows the task or needs a decision. Noticing without acting is no option.

### Self-documenting code

The code communicates its intent itself; prose is the exception. The guard rail is P.1 "[Express ideas directly in code](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rp-direct)" of the C++ Core Guidelines. What the code or a command already shows does not get documented on top.

- **Names carry the purpose, not the mechanism** — on every level: variables, functions, classes, members. Anchors: intention-revealing selector (Kent Beck, "Smalltalk Best Practice Patterns") for names; intention-revealing interfaces (Eric Evans, [DDD Reference](https://www.domainlanguage.com/ddd/reference/)) for interfaces; [general naming rules](https://google.github.io/styleguide/cppguide.html#General_Naming_Rules): optimise for readability, no cryptic abbreviations.
- **Structure explains itself:** small units with one responsibility; one level of abstraction per function (SLAP); deep modules — a small interface with much functionality behind it (John Ousterhout, "[A Philosophy of Software Design](https://web.stanford.edu/~ouster/cgi-bin/book.php)").
- **Comments are sparing** and explain the non-obvious why alone (a decision, a trade-off), never the what ([NL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-naming) of the Core Guidelines). A comment describing *what* the code does is a hint to make the code clearer — not to keep the comment.

Concretely, after [Google C++ Style](https://google.github.io/styleguide/cppguide.html#Naming) (in force):

- Types: `PascalCase` (`DateCategory`).
- Functions and methods: `PascalCase` (`GetCategoryMax()`); trivial accessors and mutators may carry `snake_case` like their member (`set_count()`).
- Class data members: `snake_case` **with a trailing underscore** (`date_format_`). Struct members without one. The clang-tidy gate enforces this member rule ([readability-identifier-naming](https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html) in `.clang-tidy`); a member without an underscore breaks the build.
- Locals: `snake_case`. Constants and enumerators: `kPascalCase` (`kColorScale`).
- The store suffix is uniformly `…Store` (not `…Storage`) — for types **and** for member and parameter names (`…_store`, not `…_storage`).

### Design principles

Binding design principles. As a general C++ guideline the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#main) hold throughout, and many of the anchors below come from them.

- [Single responsibility principle](https://en.wikipedia.org/wiki/Single-responsibility_principle) and [separation of concerns](https://en.wikipedia.org/wiki/Separation_of_concerns); low [coupling](https://en.wikipedia.org/wiki/Coupling_%28computer_programming%29), high [cohesion](https://en.wikipedia.org/wiki/Cohesion_%28computer_science%29).
- [Domain-driven design](https://www.domainlanguage.com/ddd/reference/) — see the [domain pattern](#domain-pattern-value-objects-and-stores); the layering stands under [Layers](#layers).
- [DRY](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself) as DRY of knowledge, with [single source of truth](https://en.wikipedia.org/wiki/Single_source_of_truth) as the measure: every piece of knowledge (a rule, a constant, a domain decision) has exactly one authoritative representation — not every similar-looking line folded together. But: [duplication is cheaper than the wrong abstraction](https://sandimetz.com/blog/2016/1/20/the-wrong-abstraction); two coincidentally identical blocks expressing *different* concepts stay apart — in doubt, do not abstract early ([YAGNI](https://en.wikipedia.org/wiki/You_aren%27t_gonna_need_it), [KISS](https://en.wikipedia.org/wiki/KISS_principle)).
  - The mechanics: where the same multi-line shape recurs across several methods (or panels), pull it up into a small helper — a `private` member, a free function or a shared base class — instead of copying it. Established examples: `scene_shapes::FillRectangles` and `AddCenteredText` (scene node creation), `runtime_options_detail::FoundString` (read an option → `std::optional<std::string>`), `MakeOwned<T>` (parent-owned widgets), `TablePanelBase` (the table plus add and delete scaffold), `serialization_detail::ColorToArray` and `ColorFromArray` (glm::vec4 marshalling). Prefer that over macros, because macros worsen readability and debuggability — the explicit, field-by-field `save`/`load` pairs in `infrastructure/persistence/value_serialization.hpp` stay written out on purpose, because they document the on-disk format.
- [Principle of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment): names, signatures and behaviour fit together.
- Choose the smallest useful abstraction; prefer explicit data flow over hidden coupling ([law of Demeter](https://en.wikipedia.org/wiki/Law_of_Demeter)); encapsulate unwieldy constructs instead of spreading them.
- [GRASP](https://en.wikipedia.org/wiki/GRASP_%28object-oriented_design%29) heuristics for assigning responsibility when deciding where code belongs: information expert, creator, controller, low coupling and high cohesion, indirection, pure fabrication, polymorphism and protected variations.

## Purpose

A C++26 desktop application for calendars and timelines. Grown out of a prototype; it still carries technical debt (god classes, unclear ownership). The goal is evolutionary refactoring — stepwise, behaviour-preserving, without a rewrite.

Carried by Qt 6 (GUI), OpenGL through libepoxy (rendering), ICU (calendar and locale), Boost.Serialization (XML project files), FreeType, csv2 and Bullet (picking). The full list including submodules stands in `CMakeLists.txt` and `external/` (current state through `git submodule status`). Build, tests, headless runs and the lint gate commands: [operations.md](operations.md).

## Architecture

### Layers

Provisional; the open questions stand in [#99](https://github.com/schneeregenflocke/decade/issues/99). The shape is [Ports and Adapters](https://www.hexagonalarchitecture.org/) (Cockburn): a core, ports where the direction has to turn, adapters around it, and one configurator that plugs them together. The layer names are Evans'; the one binding rule is Martin's dependency rule — dependencies point towards the domain.

- **Domain** (`domain/`) — value objects, stores, state topics. It knows QtCore and glm alone. The test asks what a library binds to, not its name: it drops out when it ties the domain to a widget toolkit, a graphics context or a file format; a Qt signal needs a `QObject`, which is QtCore ([#58](https://github.com/schneeregenflocke/decade/issues/58), [#86](https://github.com/schneeregenflocke/decade/issues/86)).
- **Application** (`application/`) — the core around the domain: project document, event bus, locale services; in `application/calendar/` the title editor, the interaction controller and the rendering adapter (`CalendarPage` with its scene composer), the one bridge into the renderer.
- **Infrastructure** (`infrastructure/`) — driven adapters: rendering (OpenGL, FreeType), persistence (XML, CSV, PNG), picking (Bullet). It knows the domain types it works on, never presentation.
- **Presentation** (`presentation/`) — driving adapters: the main window, the panels, the GL canvas. A panel owns its widgets and declares a signal for what the user edited; the window holds no store, no bus, no runtime options.
- **Configurator**, the composition root — `main.cpp`, `decade_app`, `AppComposition`, `app_binder`, `startup_script`, filed in `application/`. It builds, owns and wires everything, so it alone may know every layer (Martin's Main).
- `common/` — debug log, licence texts, embedded resources, used by the outer layers.

What the list does not show:

- **Ports:** stores publish on injected state topics (`domain/state_topics.hpp`); the rendering adapter asks for a repaint through `RenderSurface`, which the canvas implements. Persistence has no port — `ProjectDocument` calls `project_io` and `csv_io` directly, Evans' layering, until a second caller or a test needs one.
- **The GL canvas** (`GLCanvas`, `MouseInteraction`) is the one part of presentation that depends on GL. As the `QOpenGLWidget` it owns the context and what lives in it — renderer, camera, PNG export — because a GL object has to die while the canvas can still make its context current. The rest of presentation sees the scene as a GL-free snapshot.
- **Lifetimes:** the configurator owns every one; the binder wires with free functions, so components do not know each other. Whatever needs the GL context (rendering adapter, wiring) sits in an `optional` and goes when the window closes.
- **One place each:** the command line gets read in `runtime_options` alone, and the application reads no environment variable; the `LocaleDateFormatter` exists once; the open project is one object, `ProjectDocument`, and loading, saving and CSV exchange go through it.
- **Serialisation** works over public APIs alone and owns the on-disk format.
- **No include cycle** among the components (Lakos' levelization); keep it that way.

### Domain pattern: value objects and stores

- **[Value objects](https://martinfowler.com/bliki/ValueObject.html)** hold data plus the queries on it: data members `private`, const accessors, named setters. No signal, no serialisation, no `friend` → [rule of zero](https://en.cppreference.com/w/cpp/language/rule_of_three), freely copyable.
- **Stores** pair a value object with an injected state topic and a re-entry guard; they have identity and are not copyable. A store offers `Receive`, `Send` and `Get` and publishes with `Publish`. A topic is a small `QObject` carrying one value on one `Published` signal — one class per value, because moc processes no class template. The two topics carrying a `PickId` sit in `application/interaction_topics.hpp`, since that type lives in infrastructure.
- Value object and store live in **separate files**, named after the class (`date_category.hpp`, `date_category_store.hpp`).

### Date and interval semantics

- `Date` (proleptic Gregorian, with an explicit invalid state) and `DatePeriod` are the interface. ICU stays confined to two places: the arithmetic backend and the `LocaleDateFormatter`. Nobody else calls an ICU date API.
- Persisted as an [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) string ([#48](https://github.com/schneeregenflocke/decade/issues/48)).
- `DatePeriod` is **half-open `[begin, end)`** throughout ([EWD831](https://www.cs.utexas.edu/users/EWD/transcriptions/EWD08xx/EWD831.html)); a period with `end <= begin` is null, and the stores discard it. Users think in inclusive dates, so the conversion happens at two borders alone — the date table panel and CSV I/O — through `PeriodFromInclusiveDates()` and `Last()`. No ±1-day arithmetic anywhere else.

### Event flow

One typed topic per domain event on an in-process bus; the consumers get wired in `app_binder::Bind`. New state takes a value object and a `*Store` in `domain/`, a topic in `domain/state_topics.hpp`, a panel, the topic as a member of `EventBus` and its consumers in the binder; persisted state adds `save`/`load` in `infrastructure/persistence/value_serialization.hpp` and a line in `project_io`.

- **Two directions.** A panel edit is a command and goes straight to the owning store's `Receive*`; the new state is a fact and goes over the bus. Panel signals carry the edit's name (`…Edited`, `…Chosen`), topics the fact's (`Published`). A producer in presentation without an injected topic (font, tree selection) gets connected to the topic's `Publish` by the binder. The scene tree only displays.
- **Bursts.** `state_burst` brackets a run of store changes (loading a project fills six) (RAII `StateBurst`), so a consumer may rebuild once; ignoring it stays correct.
- **Delivery order is connection order** ([QObject::connect](https://doc.qt.io/qt-6/qobject.html#connect)). The shape store has to answer a new date category before the rendering adapter rebuilds, so reordering `app_binder` changes behaviour ([#97](https://github.com/schneeregenflocke/decade/issues/97)).
- **Wiring is a lifetime.** `AppWiring` connects on construction; every connection carries a `QObject` member as its context object, so destruction releases them all. The `std::function` callbacks (a pick answers with a hit, a signal cannot) release in `ReleaseCallbacks`.
- **Canvas editing.** `TitleTextEditor` holds a buffer and publishes a GL-free `TextEditView` per key; Enter sends one command to the store, Esc discards. The canvas translates key codes into a `TextInputEvent`.

## Decisions

### Language standard and header guards

- C++26, no compiler extensions. The move up from C++23 was made for [`#embed`](https://en.cppreference.com/cpp/preprocessor/embed), which carries the shaders and licence texts into the binary and replaced a submodule with a generator binary ([#79](https://github.com/schneeregenflocke/decade/issues/79)). Before C++26 both GCC and Clang grade it as an extension and `-Wpedantic -Werror` rejects it, so the standard level is not cosmetic here.
- Header guards use the file name style: the upper-cased file name with the dot before the suffix as `_`, for instance `main_window.hpp` → `MAIN_WINDOW_HPP`, `gl_canvas.hpp` → `GL_CANVAS_HPP`. No directory path prefix. Apply that consistently in `#ifndef`, `#define` and the closing `#endif  // <GUARD>` comment. (The clang-tidy check `llvm-header-guard`, which would otherwise force a full-path style, is switched off in `.clang-tidy` — leave it that way.)

### Style

Language and documentation rules live in the superproject (`~/code/homelab-superproject/AGENTS.md`).

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
- Where null is no valid value, say so with a **reference**, not with an annotation. That is the stronger statement and needs no library. Whether `gsl::not_null` says anything a reference cannot is still open in [#42](https://github.com/schneeregenflocke/decade/issues/42), `span` on the read-only container parameters in [#81](https://github.com/schneeregenflocke/decade/issues/81); until they close, add no blanket annotation.

### Components: a header and its translation unit

Every header pairs with a translation unit of the same name — the **component** after John Lakos ("Large-Scale C++ Software Design"). The header declares, the `.cpp` defines. A member gets **declared** in the class body and **defined** out of line in the component's `.cpp`: the body then reads as an interface, and editing a definition rebuilds one unit instead of the program. The move off the earlier header-only design, with the measurements that carried the decision, runs in [#88](https://github.com/schneeregenflocke/decade/issues/88).

Whatever the language holds visible stays in the header and needs no argument: templates, `constexpr` and `consteval` constants, a member returning a deduced type (`auto&`), and anything a consumer must instantiate itself. Two further headers carry no unit because they define nothing at all: `freetype.hpp` and `unicode.hpp` are entry points that gather a library's headers. Everything else needs a reason at the place itself to stay there. A function that remains in the header at namespace scope carries `inline` itself, against ODR violations once a second unit includes it. Cutting one header into smaller headers stays welcome.

One shape resists the rule: the destructor of a **pure interface** stays defaulted in the class body. Defining it out of line would give the vtable a home, but it also makes the class more than an interface — and `misc-multiple-inheritance` then fires at whoever inherits it beside a widget base, which is exactly what `RenderSurface` is for. Where a base gets inherited singly, the out-of-line destructor is welcome (`Drawable`).

`AUTOMOC` is **on**, and whoever declares a signal carries `Q_OBJECT`: the state topics, the panels, the main window. With a translation unit per header the unit [moc](https://doc.qt.io/qt-6/moc.html) emits costs nothing the design forbids. Reach for it to *declare* a signal alone — subscribing to a Qt widget's own signal works through `QObject::connect` with a functor and needs none of ours. [moc's bounds](https://doc.qt.io/qt-6/moc.html) shape the design where they bite: besides the class template (see [Domain pattern](#domain-pattern-value-objects-and-stores)), it wants signal parameters spelled out fully, so they stand as written types, not as aliases.

`emit` comes from `<QtCore/qtmetamacros.h>`, and the unit that emits includes it — Qt's own class headers do not satisfy `misc-include-cleaner` for a macro.

### OpenGL under Qt

Three traps, all silent, each settled in one place.

**libepoxy has to be first.** `epoxy/gl.h` claims the include guards `__gl_h_` and `__glext_h_` and refuses to compile once `GL/gl.h` got there before it (an `#error`). Qt's `qopengl.h` — which `QOpenGLWidget` pulls in — includes exactly those, so a header that reaches Qt first breaks the build. With the guards taken, Qt's GL headers collapse to nothing and every `gl*` call resolves through epoxy's dispatch — [which is what Qt's "avoid calling the functions directly" is actually about](https://doc.qt.io/qt-6/opengl-changes-qt6.html): it warns against link-time symbols against a linked GL library, and epoxy is itself a loader.

**A context is current in three callbacks alone.** `QOpenGLWidget` makes it current for `initializeGL`, `resizeGL` and `paintGL`, and it renders into a framebuffer object of its own — so framebuffer 0 is not the screen, and `glGetIntegerv(GL_VIEWPORT)` in a mouse handler asks nothing. Three rules follow, and each has a place that keeps it: whoever builds or destroys GL objects outside those callbacks brackets it with `GLCanvas::MakeGraphicsCurrent` (never with a `doneCurrent` — that would pull the context out from under Qt when it runs nested); whoever binds a framebuffer restores the one that was bound rather than 0 (`ScopedFramebufferBinding`); and whoever needs the viewport takes it as a parameter (`MouseInteraction`). A platform that carries no GL widget at all — the `offscreen` plugin — reports through the failure path instead of dying at the first call.

**Alpha has to leave the framebuffer at 1.** The canvas draws an opaque page, but `GL_SRC_ALPHA` blends the alpha channel with itself, so a translucent fill leaves a value below 1 behind — colour that is already final beside an alpha that means nothing. The buffer does not stay inside: `QOpenGLWidget` hands it to the compositor, and the export writes it into the PNG. Whoever reads it as premultiplied divides the colour by that alpha and wraps at 8 bit, which is how teal came out red ([#93](https://github.com/schneeregenflocke/decade/issues/93)). Hence `glBlendFuncSeparate` with `GL_ONE` on the alpha channel in `GLCanvas::ApplyInitialGlState`: colour blends as before, alpha saturates. Retagging a read-back afterwards would patch one consumer and miss the rest.

### Licences and the notices they oblige

Decade is MIT, and every dependency but one is permissive. Qt is the exception: Decade uses it under **LGPL-3.0-only**, linked dynamically and unmodified. That choice carries weight. Qt also offers GPL-2.0, and taking it would pull FreeType onto its GPLv2 branch — the FTL carries a credit clause GPLv2 does not admit, which is why FreeType comes dual-licensed at all — and the binary would end up GPLv2, with source obligations for everything in it. LGPLv3 asks for less: [§4](https://www.gnu.org/licenses/lgpl-3.0.html) wants the library named, its copyright shown among the ones the running program displays, and both the LGPL and the GPL text shipped along. The licence dialogue is where those three happen, which is why the texts travel inside the binary rather than beside it.

What no licence text says by itself stands in `licenses/notices.txt`: the Qt notice with its relinking statement, and FreeType's mandatory disclaimer ("based in part of the work of the FreeType Team").

The obligations follow the linker, not the repository layout — a system library obliges as much as a submodule, and the dialogue once showed the submodules alone. Whoever adds a dependency does four things: drop its licence text into `licenses/`, add a line to `src/common/third_party_licenses.hpp`, add the name to `tests/common/test_third_party_licenses.cpp`, and write a `sbom_add(PACKAGE …)` beside the `find_package` call it arrived with. No gate catches a forgotten one.

### Warnings, the clang-tidy and the sanitizer gate

- **Warnings break the build — fix them, do not suppress them. That rule holds for compiler warnings and for [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) diagnostics alike.** Never quieten a finding with [`NOLINT`, `NOLINTNEXTLINE` or `NOLINTBEGIN`](https://clang.llvm.org/extra/clang-tidy/index.html#suppressing-undesired-diagnostics), a `#pragma`, a `-Wno-…` flag or a single-line exception in `.clang-tidy`. Change the code so the finding no longer takes hold. Restructuring, RAII and correct type annotations are fixes; suppression is not.
  - **Prefer a real fix, even where the warning looks "unfixable" at first.** Usually it is not. An example: [`cppcoreguidelines-owning-memory`](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/owning-memory.html) over a raw C resource is satisfied once ownership is expressed with `gsl::owner<>` from the [GSL](https://github.com/microsoft/GSL) — the project already links `Microsoft.GSL::GSL` — and a `std::unique_ptr` with a custom deleter; see `src/infrastructure/graphics/png_writer.cpp`. A rule that truly does not apply to the project style gets switched off **once, globally** in `.clang-tidy` with a comment (as `-modernize-use-trailing-return-type` and `-llvm-header-guard` already are) — never spread per line.
  - **Suppression is a last resort alone, and only for constructs a third-party C API forces on us contractually and that the code cannot solve otherwise.** Two stand today: libpng's mandatory `setjmp`/`longjmp` error handling in `src/infrastructure/graphics/png_writer.cpp`, and FreeType's macro include in `src/infrastructure/graphics/freetype.hpp` — `FT_FREETYPE_H` exists only once `<ft2build.h>` has run, and the order `llvm-include-order` demands breaks the build. If you must suppress: scope the `NOLINT` to the **concrete check names** (never a bare `NOLINT`), confine it to the narrowest line and add a comment explaining *why* it is unfixable. Where a whole dependency makes a class of warning unavoidable, replace that dependency rather than spread suppressions.
  - The one `-Wno-…` in the build is **not** an exception to the rule but a statement about a compiler: `-Wno-c23-extensions` says that Clang 22 has not yet implemented `#embed` as the C++26 feature it is. It says nothing about our code — GCC compiles the same line as standard under `-std=c++26 -Wpedantic -Werror`. It stands in the clang branch of both build files and goes the day Clang catches up.

One build directory, one compiler in it: `build/` holds GCC or clang, chosen when you configure, and the choice decides which targets exist — `clang-tidy` and `sanitize-memory` appear under clang alone, because they parse with its frontend — and a GCC `compile_commands.json` carries `-mno-direct-extern-access` out of `Qt6::Platform`, which clang rejects as an unknown argument before any check runs. `CMakeLists.txt` gathers each compiler's own flags and targets into one section at its end; whatever holds for both stands before them, unguarded. `CMakeLists.txt` and `tests/CMakeLists.txt` share no target and no variable — each names its own sources and its own dependencies, so either reads on its own and a new dependency has to be entered twice. CI runs one job per compiler, and both build and test, because each sees warnings the other does not. Whoever adds a gate says which compiler it belongs to.

Beside clang-tidy stands the sanitizer gate `sanitize-address` (address, leak, undefined): it rebuilds the tree instrumented and runs the test suite underneath. It gets held at **zero findings**; a sanitizer hit gets fixed, not suppressed.

The gate commands (enforcement targets, the full run, auto-fix, clang-format, CI) stand in [operations.md](operations.md), section Build checks.
