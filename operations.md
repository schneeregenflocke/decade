# decade — build and operations

Build, tests, headless runs and the lint gates. Principles, architecture and conventions stand in [AGENTS.md](AGENTS.md).

## Operations

### Build

#### Building

The build runs on [CMake](https://cmake.org/cmake/help/latest/) with the [Ninja](https://ninja-build.org/manual.html) generator. The build directory is `build/`; `compile_commands.json` gets exported for clangd and clang-tidy.

Initialise the submodules once after cloning (their state at any time through `git submodule status`):

```bash
git submodule update --init --recursive
```

Reconfigure (only when `CMakeLists.txt` or a dependency changes):

```bash
cmake -S . -B build -G Ninja
```

Build (from the repository root; the Ninja file sits in `build/`):

```bash
ninja -C build
```

Start the GUI:

```bash
./build/decade
```

#### Tests

`tests/` mirrors the `src/` structure; the wx-free headers are unit-tested directly. After building:

```bash
ctest --test-dir build
```

### Startup file

A startup file (CSV or XML) gets passed as a positional argument alone; without one, an empty project starts.

```bash
# CSV import
./build/decade examples/sample_dates.csv
```

### Headless runs

The binary honours several command line options in GNU syntax (`--name=value` or `--name value`; `--help` shows them all) for non-interactive use — CI, screenshots, smoke tests. The option vocabulary is defined and parsed in exactly one place (`AddRuntimeOptions` and `RuntimeOptionsFromParser` in `src/application/runtime_options.hpp`); the binary reads no environment variables any more.

**Image capture** — two options capture two different things; they are not redundant:

- `--dump-png=<path>` — the calendar **page image** alone, through an off-screen FBO. Resolution: the export DPI, a white background, no app chrome. Needs: OpenGL alone.
- `--dump-frame-png=<path>` — the **whole window**: tabs plus panels (a `wxClientDC` blit) with the content composed on top of the GL back buffer. Resolution: screen resolution. Needs: widget read-back, which works on X11 and Xvfb alone and comes out empty under Wayland.

Take `--dump-png` for a clean high-DPI export of the page itself, `--dump-frame-png` for the real GUI including chrome. `--dump-frame-png` gets delayed through `CallAfter`, so the first paint has already happened.

**The two channels.** A run without `--debug-log` writes **nothing** on stdout, and on stderr only what the user has to act on: a startup file that would not load, an option value that got ignored, a shader that would not compile, a device that failed. Everything else — progress, versions, sizes, frame rates, the shader inventory — is diagnosis and hangs on `--debug-log`. A silent run therefore means "nothing to report", and any line at all is worth reading. The switch itself lives in `decade_debug::LogEnabled()` (`src/common/debug_log.hpp`), which `DecadeApp::OnInit` sets from the option.

**Steering:**

- `--dump-png-dpi=<dpi>` — the export DPI for `--dump-png`; `GLCanvas::kExportPngDpi` (200) when unset. Used for high-resolution README renderings, for instance.
- `--exit-after-ms=<ms>` — closes the main window after N ms by itself.
- `--select-tab=<label>` — preselects a notebook tab by label at start (case-insensitive), for screenshotting a particular tab.
- `--debug-log` — switches on OpenGL and runtime debug logging. It also forwards wx **assert failures to stderr and keeps running** instead of opening a modal dialogue, so headless and screenshot runs make a failing `wxASSERT` visible (instead of blocking in silence) — see `DecadeApp::OnAssertFailure`.
- `--debug-hover-bar=<index>` — highlights the bar with the given index at start as though it were hovered, to screenshot or debug the hover path without a live cursor.
- `--debug-hover-title` — the same for the title frame; it beats `--debug-hover-bar`, because at most one element is ever hovered.
- `--debug-edit-title=<text>` — opens the title edit after loading, like a double click, and types `<text>` into it; empty means open alone, with everything selected. The edit stays open, so cursor and selection stand in the image.
- `--debug-select-node=<path>` — selects the scene tree node at `path` (`root/.../name`) at start and thereby walks the real selection path (the scene tab detail grid plus the calendar selection highlight of the node and its subtree) without a pointing device.

A typical smoke test (passing the sample data explicitly):

```bash
stdbuf -oL -eL timeout 12 ./build/decade \
  --dump-png=/tmp/decade_render.png --exit-after-ms=2000 examples/sample_dates.csv
```

A full UI screenshot (tabs plus panels plus canvas) of a particular tab. The widget read-back works on the **X11 backend** alone — a `wxClientDC` blit delivers black under Wayland — so run it headless under **Xvfb** with software GL. That is the supported way to screenshot the real GUI: GNOME and Wayland block programmatic screen capture, and `GDK_BACKEND=x11` on a running XWayland session breaks the EGL surface of the GL canvas.

```bash
xvfb-run -a -s "-screen 0 1600x1000x24" \
  env GDK_BACKEND=x11 LIBGL_ALWAYS_SOFTWARE=1 \
  timeout 30 ./build/decade --select-tab=Timeframe \
  --dump-frame-png=/tmp/decade_ui.png --exit-after-ms=3000 examples/sample_dates.csv
```

### Build checks

The rule behind them — **warnings break the build, never suppress them** — stands in [AGENTS.md](AGENTS.md), section "Warnings, the clang-tidy and the sanitizer gate". Here are the commands.

**The enforcement gate (a build breaker).** It runs clang-tidy on the single translation unit `src/application/main.cpp` — which covers every `src/` header transitively through `HeaderFilterRegex` — and grades every finding as an error:

```bash
cmake --build build --target clang-tidy   # fails on EVERY finding
```

The tree gets held at **zero findings**; a new finding breaks this target. The gate deliberately sits *outside* the standard build, so normal compiles stay fast — run it explicitly or in CI. Prefer this single-TU form over globbing `src/**/*.hpp` directly: analysing headers in isolation creates artificial `misc-include-cleaner` noise for the GL and wx umbrella headers that never turns up in a real translation unit. Every check switched off in `.clang-tidy` carries a comment explaining why (glm unions, GL and wx C API interop, deliberate style).

The full run (a report in `build/clang-tidy.log`):

```bash
clang-tidy -p build \
  --extra-arg=-include --extra-arg=type_traits \
  --extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option \
  src/**/*.cpp src/**/*.hpp 2>&1 | tee build/clang-tidy.log
```

Auto-fix for a single check group (`modernize-*` for instance, without `--fix-errors`):

```bash
clang-tidy -p build \
  --extra-arg=-include --extra-arg=type_traits \
  --extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option \
  --fix --fix-notes \
  --checks='-*,modernize-*,-modernize-use-trailing-return-type' \
  src/**/*.cpp src/**/*.hpp
```

Why the extra args:

- `-include type_traits` works around a bug in `wx/meta/convertible.h`, which uses `std::is_base_of` without including `<type_traits>`. GCC pulls it in transitively, Clang does not.
- `-Wno-error` and `-Wno-unknown-warning-option` keep GCC-specific flags from `compile_commands.json` (`-Wlogical-op`, `-Wduplicated-branches` …) from turning into compiler errors in clang-tidy together with `-Werror` from the build.

`src/**/*.cpp src/**/*.hpp` presumes the shell supports recursive globs (zsh by default; bash only after `shopt -s globstar`).

#### Sanitizers

The second gate beside clang-tidy: the code runs instrumented instead of merely being read. Both targets configure a build folder of their own under `build/`, rebuild everything in it and run `ctest` there — a run finds errors, a build alone does not. Flags: [GCC, instrumentation options](https://gcc.gnu.org/onlinedocs/gcc-9.2.0/gcc/Instrumentation-Options.html).

```bash
cmake --build build --target sanitize-address   # the gate run, see below
cmake --build build --target sanitize-memory    # memory (clang) — diagnosis, see below
```

- **`sanitize-address`** is the gate. It combines AddressSanitizer (buffer overruns, use-after-free), LeakSanitizer and UndefinedBehaviorSanitizer. The tree gets held at **zero findings**. `-fno-sanitize-recover=undefined` is needed, because UBSan would otherwise merely report and carry on — the gate would stay green. LeakSanitizer already sits inside AddressSanitizer on Linux; it is named anyway, so the intent stands in the target.
- `_GLIBCXX_ASSERTIONS` rides along. It is no sanitizer, but it closes a hole they leave: reading past a container's size while it still has capacity stays inside the allocation, so AddressSanitizer sees nothing — `v.front()` on an empty vector after `reserve()` hands back garbage and runs on.
- `float-cast-overflow` and `float-divide-by-zero` stand **beside** `undefined`, because GCC folds neither into it (checked against GCC 16 on 2026-08-06 — a cast of 7.87e30 to `size_t` passes unremarked under plain `-fsanitize=undefined`). Whoever extends the flag list checks the same way: write the smallest program that triggers the class, compile it with the gate's flags and see whether it fires. A flag nobody has seen fire buys nothing.
- **`sanitize-memory`** is a diagnostic tool, not a gate. MemorySanitizer (uninitialised reads) excludes AddressSanitizer — the compiler rejects the combination — and clang alone knows it, hence a second target. It demands that **every** dependency be instrumented; with the system libstdc++ and gtest it reports false alarms out of foreign code and breaks off during test discovery already. It would become usable only with a self-built, instrumented libc++ plus a rebuilt gtest, ICU and Boost.
- `embed-resource` runs during the build and is exempt through `-fno-sanitize=all`: a finding in the tool would break the build instead of checking the program.
- The GUI binary in the sanitizer folder (`build/san-address/decade`) starts under Xvfb as usual; the GL drivers hold memory on exit, so a leak report out of that says little.

#### clang-format

`.clang-format` is the binding source for the formatting ([ClangFormat documentation](https://clang.llvm.org/docs/ClangFormat.html)) and says one thing: `BasedOnStyle: Google`, without a single deviation. Keep it that way — a `--dump-config` in that file names every option of the version that wrote it, and an older clang-format then aborts on the first unknown key and silently formats by the LLVM default instead.

Format the tree:

```bash
find . -regex '.*\.\(cpp\|cxx\|hpp\|cc\|h\)' -not -path './build/*' -not -path './external/*' -exec clang-format -style=file -i {} +
```

Ask instead of change — the same command CI runs:

```bash
git ls-files '*.hpp' '*.cpp' | grep -v '^external/' | xargs clang-format --dry-run -Werror
```

Formatting commits stand in `.git-blame-ignore-revs`, so `git blame` walks past them to the change that gave a line its meaning. Every clone activates that once:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

#### CI

CI runs on our own [Forgejo instance](https://git.blem.ch/), not on GitHub Actions. The old workflow file `.github/workflows/cmake.yml` runs on demand alone (`on: workflow_dispatch`, so `gh workflow run cmake.yml`) and takes part in no push. The chain, analogous to `blem-website`:

- `git push origin main` (GitHub) → the GitHub webhook calls the `mirror-sync` API → the pull mirror `github-mirror/decade` syncs at once → the sync fires the push event for `.forgejo/workflows/build.yml`.
- The workflow (`runs-on: runner-laptop-omen`) runs in an `ubuntu:26.04` container **exclusively** on the CI runner on `laptop-omen` — `runner-<host>` is the exclusive runner label (against the shared `ubuntu-ci`). The heavy C++ build (Boost, wx, submodules) must not land on homelab (an RPi 4 on USB SMR): there it fails on the 1200 MB limit and drives the disk into I/O saturation (the loadavg incident of 2026-07-11). Is the laptop off, the job waits in the queue instead of falling back to homelab. The runner stack: `docker-stacks/forgejo-runner/docker-compose.ci.yml`, deployed through Komodo (stack `forgejo-runner-laptop-omen`); the label scheme sits in the README there.
- Steps: apt dependencies → checkout with submodules → the clang-format gate → `cmake` and `ninja` (with `-Werror`) → `ctest` → the clang-tidy gate → `sanitize-address`.
- The clang-format gate runs first, because it needs no build. It exists because the pre-commit hook does not gate anything: it lives in the clone and only after `pre-commit install`, and the drift of [#63](https://github.com/schneeregenflocke/decade/issues/63) grew in a clone that had never run it.
- `sanitize-address` builds the tree a second time — one CI run therefore compiles the project twice. `sanitize-memory` deliberately does not run in CI: it breaks off on the uninstrumented libstdc++ and gtest and stays a local diagnostic tool.
