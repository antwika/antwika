# Suggestions

The ranked backlog of source-code improvements the agent loop works from.
`WORKFLOW.md` describes how an iteration selects from this file; the short version is that entries are taken top-first, no two agents in one iteration may touch the same file, and an entry without a `VERIFY` command is not eligible to be dispatched at all.

Each entry carries `WHERE` (paths an implementer may touch), `WHAT`, `FIX`, `SIZE`, `RISK` and `VERIFY`.
An entry marked **UNVERIFIED** was proposed from a reading that the auditor could not finish; the implementer's first job is to confirm the problem is real, and to close the entry as "not a defect" if it is not.
That is a successful outcome, not a failed one.

Every `VERIFY` command below assumes the dummy-driver environment described in `LESSONS.md`:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build --output-on-failure
```

---

## 0. The determinism gate — the standing top priority

This entry sits outside the ranking because it is larger than one iteration and because every other thing this project might become depends on it.
It is deliberately not dispatched as a single agent task; an iteration takes one numbered step of it at a time.

- **WHERE**: new work, most likely `src/libs/replay/` plus a `scripts/` driver and a CI step in `.github/workflows/build.yml`
- **WHAT**: The repository's central claim is that a run is reproducible from its recorded input, across three toolchains and every backend.
  That claim is currently defended by design argument alone — by `ITickSource`, by integers everywhere, by `pathfinding`'s total ordering, by `PointerHintChannel` staying out of the stream — and **not by any test that can fail**.
  `IDEAS.md` says as much itself.
  `REVENUE.md` reaches the same conclusion from the other direction: every proposal there that could earn money rests on this claim, and the first sceptical reader asks to see the cross-toolchain golden-hash test.
- **FIX**, in the order an iteration should take it:
  1. A `state hash` for one app's simulation state — `apps/life` is the right first subject, being the smallest — computed from committed state only, never from anything render-side.
  2. A test that replays a checked-in demo replay and asserts the per-tick hash sequence against a golden file.
  3. The same for `apps/game`, which is the app where the hard cases live (the camera, the walker interpolation, the A* tie-break).
  4. A CI step running the golden comparison on all three legs, so GNU, LLVM and MinGW must agree on the same file.
- **SIZE**: large, and the only large entry in this file.
- **RISK**: the hash must cover exactly what a replay reproduces and nothing else.
  A hash that accidentally includes a render-side value (a `WalkerSprite`, a `FrameMeter` reading, a hover) will fail intermittently and teach everyone to distrust the gate, which is worse than not having one.
- **VERIFY**: the golden test fails when a deliberate one-tick perturbation is introduced, and passes otherwise — an agent implementing a step must demonstrate both.

---

## 1. `life` draws against the reported window size and hit-tests against the configured one

- **WHERE**: `src/apps/life/src/RenderSystem.cpp:22`, `src/apps/life/src/PointerToggleSink.cpp:31,95`, `src/apps/life/src/main.cpp:52-55,126`
- **WHAT**: `RenderSystem::draw` calls `scene.draw(renderer, window.size(), board)` — the size the window currently *reports* — while `PointerToggleSink` is built from `kWindowSize`, the configured constant, and computes `layoutFor()` from it in its initialiser list.
  CLAUDE.md states this app's mapping is against the configured size, and the comment at `main.cpp:52-54` says so explicitly, but only the sink honours it.
  The two agree today solely because the window is created non-resizable, so nothing structurally prevents a later `resizable = true` from making the drawn board and the clickable board disagree.
- **FIX**: Call `window.configuredSize()` in `RenderSystem::draw`, so `BoardScene` and `PointerToggleSink` derive one `BoardLayout` from one number.
  Add a test with a fake window whose `size()` differs from `configuredSize()`, asserting the scene lays out against the configured one.
- **SIZE**: small. **RISK**: very low; the two values are equal under every current backend.
- **VERIFY**: `cmake --build build -j24 && SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build -R antwika_life_tests --output-on-failure`

## 2. `IWindow::configuredSize()` is non-pure, so a new window can silently inherit the wrong answer

- **WHERE**: `src/libs/gfx/include/antwika/gfx/IWindow.hpp:66,79,86`; implementers `src/libs/gfx/src/NullWindow.hpp:64`, `backends/sdl3/src/Sdl3Window.hpp:80`, `backends/raylib/src/RaylibWindow.hpp:85`
- **WHAT**: The method is declared `virtual` with an in-class default rather than `= 0`, while its own doc comment is emphatic that layout and hit-testing must use it and never `size()`.
  A default means a new window type that forgets to override it compiles and returns whatever the base chose, and the failure mode is a recorded click resolving against a different rectangle — exactly the bug the comment exists to prevent.
  This is the highest-leverage entry in the file: it converts a documented rule into one the compiler enforces, which is the project's stated preference throughout.
- **FIX**: Make it pure virtual; all three production implementers already override it.
  Fix up any test double that relied on the default, or give the doubles a shared `FakeWindow` base rather than putting a default back on the interface.
- **SIZE**: small. **RISK**: every `IWindow` test double must now define it — the compiler names each one, so nothing fails silently.
- **VERIFY**: `cmake --build build -j24 && SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build --output-on-failure`

## 3. Retire the three `antwika::replay` shims for `antwika::cli`

- **WHERE**: delete `src/libs/replay/include/antwika/replay/{CommandLine,CommandLineError,FlagSpec}.hpp`; update `src/apps/game/include/antwika/game/SaveCli.hpp:7-14`, `src/apps/poker/include/antwika/poker/WatchOptions.hpp:6-7,62,76`, `src/apps/poker/src/WatchOptions.cpp:17,24,29`, `src/apps/atlas_editor/include/antwika/atlas_editor/EditorOptions.hpp:9-10,102,118`, `src/apps/atlas_editor/src/EditorOptions.cpp:22-42,90,96`, and the tests `src/apps/game/tests/SaveCliTest.cpp`, `src/apps/poker/tests/WatchOptionsTest.cpp`, `src/apps/atlas_editor/tests/EditorOptionsTest.cpp`
- **WHAT**: The three headers are pure `using` re-exports whose own comment states the exit condition: "These go once every caller names `antwika::cli` itself."
  A grep of `src/` and `backends/` shows the only remaining users are those three apps, so the condition is one mechanical rename away.
  Until then an app that wants to read two dashes and a word still names the replay library in its public headers.
- **FIX**: Rename the call sites to `antwika::cli::` and include `<antwika/cli/...>`; add `antwika::cli` explicitly to each app's link libraries, since they may currently get it only transitively through `antwika::replay`; then delete the shims.
- **SIZE**: small. **RISK**: a missed link library breaks the build immediately rather than silently.
  These headers are installed, so this is an API removal for any external consumer — acceptable, and the reason it is ranked below the two above.
- **VERIFY**: `grep -rn "replay::CommandLine\|replay::FlagSpec\|replay/FlagSpec.hpp" src backends` returns nothing, then a full build and test run.

## 4. CLAUDE.md states two rules against symbols that no longer exist

- **WHERE**: `CLAUDE.md`, the `antwika::input` and `antwika::wfc` paragraphs
- **WHAT**: CLAUDE.md says "no `input.*` name may ever be added to an app's `kSelfGeneratedEventNames`", but `grep -rn kSelfGeneratedEventNames src` returns **zero** hits — the identifier is gone from the tree.
  It also lists `wfc::AdjacencyConstraint` as having no application, while it is in fact the backbone of both level generators (`src/apps/game/src/WorldMap.cpp:59-63,261-265`, `src/apps/tower_defence/src/LevelGenerator.cpp:180-185,301-305`).
  A rule phrased against a vanished symbol is a rule nothing can enforce, and a stale "unproven" note sends the next agent to prove something already proven.
- **FIX**: Describe the mechanism that actually thins the recording today (`InputEventCodec` plus the upstream decorators), and correct the `AdjacencyConstraint` note to name its two callers.
  Documentation only; no source change.
- **SIZE**: small. **RISK**: none to the build.
- **VERIFY**: `grep -rn "kSelfGeneratedEventNames" .` returns nothing outside this file's own history, and `python3 scripts/check_one_sentence_per_line.py` passes.

## 5. `wfc::EntropyIndex` relies on a precondition enforced only in `Solver`

- **WHERE**: `src/libs/wfc/src/EntropyIndex.{hpp,cpp}`, guard at `src/libs/wfc/src/Solver.cpp:69-74`, test at `src/libs/wfc/tests/WeightedEntropyTest.cpp:38`
- **WHAT**: **UNVERIFIED as a live defect** — the guard exists and is correct, so this is hardening rather than a bug.
  `sortKey()` computes `weight * std::log(weight)` and keys a `std::set` on the rounded result; a weight of `0.0` makes that `0 * -inf = NaN`, and a NaN key destroys the set's strict weak ordering, which is undefined behaviour rather than a wrong answer.
  The only thing preventing it is `Solver`'s `if (!(weight > 0.0)) throw`, which correctly rejects zero, negative and NaN — but `EntropyIndex` is separately constructible and states no precondition of its own.
- **FIX**: Restate the precondition where it is relied on, and add a test pinning that a NaN weight is refused.
- **SIZE**: small. **RISK**: a stricter assert could fire on a caller the auditor did not find.
- **VERIFY**: `SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build -R antwika_wfc_tests --output-on-failure`

## 6. `apps/gfx_demo` may lay out and hit-test its UI against the reported size

- **WHERE**: `src/apps/gfx_demo/src/DemoLoop.cpp:115`
- **WHAT**: **UNVERIFIED** — the call site `const auto canvas = window->size();` was seen by grep, but the surrounding loop was not read.
  `DemoLoop` is the worked example for `antwika::ui`'s clickable buttons, and CLAUDE.md states a UI's canvas must be the configured size and never the reported one.
  If that one `canvas` value feeds both the `ui::Context` layout and the pointer hit-test, the showcase demonstrates the opposite of the rule — which matters most because it is the file the next agent copies from.
- **FIX**: Read the loop; if layout and hit-test share the reported size, switch to `configuredSize()`.
  `gfx3d_demo` (`src/apps/gfx3d_demo/src/SpinLoop.cpp:75`) has no pointer input and may legitimately keep the reported size for its projection — confirm and leave it, or note why.
- **SIZE**: small. **RISK**: changes the demo's appearance on a resized window only.
- **VERIFY**: `SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build -R antwika_gfx_demo_tests --output-on-failure`

## 7. `apps/game`'s grid scene draws against the reported size

- **WHERE**: `src/apps/game/src/RenderSystem.cpp:127`
- **WHAT**: **UNVERIFIED** — the call `scene.draw(renderer, setup.window.size(), ...)` was confirmed, but `UiOverlay`'s construction was not read.
  CLAUDE.md says `UiOverlay` owns the canvas the toolbar is laid out against, being the size the window was asked for, "so nothing can lay it out against one size and hit-test it against another".
  If the grid draws against the reported size while the overlay hit-tests against the configured one, a resizable window puts the toolbar somewhere other than where it is clickable.
- **FIX**: Confirm which size `UiOverlay` holds.
  The grid may legitimately follow the reported size, since the projection anchors on the camera's pan rather than the canvas centre — in which case the correct outcome is a comment saying so, and the toolbar is the only thing that must not.
- **SIZE**: small. **RISK**: must not alter `GameSummary`; `ReplayDeterminismTest` is the gate.
- **VERIFY**: `SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build -R antwika_game_tests --output-on-failure`

## 8. Every bounded random draw is hand-written at its call site

- **WHERE**: `src/libs/rng/`, call sites in `holdem::Deck` and `src/apps/tower_defence/src/LevelGenerator.cpp`
- **WHAT**: **UNVERIFIED** — the `rng` library was read in full, the two call sites were not opened.
  `IRng::next()` returns raw bits and every bounded draw is written by hand with `%`, which is a deliberate and well-argued decision — the bias is stated rather than hidden by a non-portable distribution.
  The cost is that each call site independently gets the `+ 1` right in a Fisher-Yates swap, and a `% 0` on an empty range is undefined behaviour rather than a refusal.
- **FIX**: Do **not** add a distribution.
  Add one tested `boundedDraw(IRng &, std::uint64_t bound)` that documents the bias in one place and refuses `bound == 0`, then move the existing call sites onto it.
- **SIZE**: small. **RISK**: **high** — if the helper's arithmetic differs from `next() % bound` by one operation, every checked-in replay and every seeded level changes.
  It must be byte-identical, and the determinism tests are what prove it.
- **VERIFY**: full `ctest` run; the replay determinism and level generator tests are what would catch a changed stream.

## 9. `input::ActionMap` and `input::Binding` have no caller anywhere

- **WHERE**: `src/libs/input/include/antwika/input/ActionMap.hpp`, `Binding.hpp`; the only hits outside the library are `src/libs/input/tests/ActionMapTest.cpp`
- **WHAT**: Verified by grep across `src/` and `backends/`.
  The question this API exists to answer — does a real app want this shape? — is genuinely unanswered, which is worse than either outcome.
- **FIX**: Either wire it into one app that decodes keys by hand today (`apps/game`'s toolbar and camera keys are the natural candidate), or delete it.
  **Not both, and not in one change.**
  This needs a decision recorded in `ISSUES.md` before an agent spends a budget on it, since deleting a public API is not an autonomous call.
- **SIZE**: medium. **RISK**: wiring it touches the tick path and must not change what any recorded stream produces.
- **VERIFY**: `SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build --output-on-failure`, `ReplayDeterminismTest` in particular.

## 10. Seventeen orphaned worktree directories hold 12-13 GB and are invisible to git

- **WHERE**: `.worktrees/` — `anim, buildings, chores, ci-perf, gfxresize, i18n, menu, pathfind, port-clean, raylib-init-crash, render, replay-size-plan, truck-sprite, tts-plan, wire-anim, wire-menu, wire-path`
- **WHAT**: `git worktree list` registers four entries; `ls .worktrees/` shows twenty.
  The seventeen extra directories have a `.git` file pointing at a gitdir that no longer exists, so `git -C <dir> status` fails **silently and empty** rather than erroring.
  Any future script that sweeps `.worktrees/*/` therefore gets a false "no uncommitted changes" from them, and creating a new worktree whose name collides with an orphan fails on a non-empty directory.
- **FIX**: Confirm each orphan holds nothing unmerged, then remove the directories and run `git worktree prune`.
  The three *registered* worktrees are all on branches already merged into `main` and can go too; `port/i18n` is the only local branch with unmerged work and must be left alone.
- **SIZE**: small. **RISK**: destructive and irreversible — this one wants a human's confirmation before the deletion, and is parked in `ISSUES.md` for that reason.
- **VERIFY**: `git worktree list` and `ls .worktrees/` agree afterwards.

---

## Areas not yet audited

Three parallel audits were dispatched in iteration 0 and did not survive the process that launched them, so these areas have had no pass at all and are where the next auditor should start:

- `src/libs/sound`, `src/libs/input`, and the SDL3/raylib duplication under `backends/`.
- `src/libs/ui`'s proportional-shrink rounding, `src/libs/gfx`, `src/libs/ttf`'s font-table bounds checking, `src/libs/i18n`, `src/libs/animation`.
- The six applications: `game`, `life`, `tower_defence`, `poker`, `companion`, `atlas_editor`.
