# The solver that lied, the zombie component, and a coverage gate with teeth

*Post 8*

The [previous post](007-running-until-stop-and-the-greeting-that-was-never-really-input.md) closed out the engine's move to running until `engine.stop` instead of a fixed tick count.
This post isn't about a feature at all — it's what happens when nine libraries and four apps get read end to end by a reviewer looking for exactly the kind of bug that only shows up when someone reads the code instead of trusting that its tests are green.
A [coding style guide](../docs/STYLE_GUIDE.md) went in first, collecting conventions that had only ever lived in review comments; everything else in this post is what reading the codebase against that guide, and against its own logic, turned up.

## Three bugs that were already live

`wfc::Solver::propagate()` only recorded a constraint's domain mutations on the `Trail` when `prune()` returned `true` — but a constraint can mutate several cells before the one mutation that empties a domain and forces `prune()` to return `false`.
Those earlier mutations went unrecorded, so backtracking out of that branch left a sibling branch's domain missing candidates it should have had back, and the solver could report `Unsatisfiable` for a wave that was solvable all along.
The fix records diffs unconditionally, before ever looking at `prune()`'s return value — `SolverBacktrackingTest.FailingConstraintsPartialMutationsAreUndoneOnBacktrack` reproduces the exact `AllDifferentConstraint` + `AdjacencyConstraint` scenario that used to get this wrong.

`ecs::World::add<T>()`'s deferred insert had the same shape of problem one layer down.
If a `destroy()` staged earlier in the same `commit()` batch ran before a queued `add<T>()`, the insert went ahead anyway — a permanently orphaned "zombie" component, visible to `view<T>()` forever, attached to an entity that was already dead.
The deferred insert now re-checks `alive(entity)` at apply time and no-ops if the entity was retired first, closing the exact ordering [post 3](003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md) built `commit()` to make possible in the first place.

`replay::BinaryPrimitives::readString` and `BinaryReplayReader::read` both trusted a length or count field straight off the wire before checking the stream actually held that much data — a corrupt or truncated replay file could force a multi-gigabyte allocation attempt instead of a clean `ReplayFormatError`.
`readString` now reads in bounded 64 KiB chunks and grows a `std::string` as it goes instead of allocating the claimed length up front; the event vector in `read()` grows naturally instead of `reserve()`-ing on an untrusted count.
Both got a regression test that overwrites a real length/count prefix with `0xFFFFFFF0` and checks the read throws instead of trying to allocate four gigabytes for it.

## Five boundaries that took whatever they were given

Five places accepted a payload's happy path and never asked what an unhappy one would do to them.

`game::GameStateReducer` parsed `game.score_increment` with `std::stoull`, which accepts a leading `-` via two's-complement wraparound and silently ignores trailing garbage after a valid number — `"−1"` became a huge score instead of an error, and `"5abc"` quietly became `5`.
A `std::from_chars`-based parse now checks both the error code and that the whole payload was consumed, throwing a new `GameStateReducerError` otherwise.
`life::BoardSink`'s `life.toggle_cell` parser called `std::from_chars` and never looked at its result at all, so a missing comma or a non-numeric field produced a silently wrong coordinate; it's validated now the same way, via a new `BoardSinkError`.
`task_worker::TaskSubmissionSink` accepted two `task.submit` events sharing an id, and every later lookup by that id silently resolved to the first match — the second task's progress and completion updates were applied to the wrong registry entry the whole time.
`handle()` now rejects a repeated id through the existing `TaskSubmissionError`.
`sudoku::Board::at`/`set` indexed their backing array with no bounds check at all, unlike `Board::parse`'s already-strict validation; both now throw `BoardFormatError` for an out-of-range row or column, and `set()` rejects a digit outside `[0, 9]` too.
`wfc::CompatibilityTable::set`/`compatible` indexed a `std::vector<bool>` with no bounds check, reachable from `AdjacencyConstraint` any time a table's alphabet size didn't match the wave's — it now treats an out-of-range pair as incompatible, the same way `Domain` already handles out-of-range values, and got a dedicated `CompatibilityTableTest.cpp` since it had none before.

## Five things that were fine but not right

A second pass, lower stakes but still worth doing: every concrete class missing `final` got it — `Engine`, `SystemClock`, `FakeClock`, `EngineLoop`, `Logger`, `NullAppender`, `StreamAppender`, `Domain` and its `const_iterator`, `CompatibilityTable`, `Solver`, `Trail`, `EntropyIndex`, `Game`, `Life`, `TaskWorker`, `sudoku::Board` — sixteen classes, each checked first to confirm nothing in the repo actually subclasses it.
`Engine.cpp`'s `GCOVR_EXCL_LINE` on the tick-dispatch call had no comment explaining why it was excluded, unlike every other exclusion in the repo; it's now documented as the allocation-unwind edge it is, confirmed with `gcov -b` against a real coverage build.
`libs/event`'s public headers included their siblings by bare filename (`"Event.hpp"`) instead of the full installed path (`"antwika/event/Event.hpp"`) that `libs/ecs` already used — every header in the module was rewritten to match.
`wfc::Solver::propagate()`'s lambda allocated a fresh `queued` vector on every call, even though `propagate()` runs once per candidate-value attempt during search rather than once per `solve()`; the vector is now allocated once outside the lambda and reset with `std::fill()` per call instead.
`replay::BinaryPrimitives::writeString` had no check that a string actually fit in the 32-bit length prefix it was about to write, so an oversized payload would silently truncate that prefix instead of failing — a `checkFitsInU32Length()` helper, factored out so the boundary can be tested without allocating a multi-gigabyte string, throws `ReplayFormatError` instead.

## The check that checked itself

`scripts/check_unused_test_doubles.py` decided whether a mock or fake header was "used" with a bare substring search for its filename over a `.cpp` file's raw text — a header merely *mentioned in a comment* counted as used, and matching was filename-only with no path scoping, so an unused mock in one module could hide behind a same-named, genuinely-used mock in a different one.
It's a regex-based `#include` parse now, matching the full installed path a real consumer would actually write, with regression tests for exactly the two failure modes above plus one proving angle-bracket includes still match.

Two smaller scripts had their own bugs.
`check_one_sentence_per_line.py`'s shebang guard checked the character *before* a found `#` for `!`, but a real `#!` sequence has the `!` *after* the `#` — the guard was a no-op for every actual shebang, so a `"#!"` sitting inside a string literal got wrongly treated as a comment start.
It checks the correct side now.
Both `check_line_length.py` and `check_one_sentence_per_line.py` also read source files with no explicit encoding, leaving decoding dependent on the process locale; both now pin `encoding="utf-8"`.

## CI: a real race, some real timeouts, and layers that get reused

The coverage-badges job had a genuine race: two pushes to `main` close together could both fetch, commit, and push the orphan `badges` branch at once, and the second push would fail as a non-fast-forward — reporting an unrelated run as red for a reason that had nothing to do with its code.
A `concurrency` group (queued, not cancelled) on that job fixes it.
Every job with its own `runs-on`/steps picked up a `timeout-minutes`, so a stuck Docker build, Conan install, or `ctest` run can't occupy a runner indefinitely; the two `workflow_call` jobs in `ci.yml` and `release.yml` were left alone, since `timeout-minutes` isn't valid alongside `uses:` on a reusable-workflow-calling job — their own inner jobs are bounded individually instead.
Conan's `~/.conan2` is now cached per matrix profile, and Docker layers for each image build are cached through the GitHub Actions cache backend, scoped per image so they don't evict each other — gtest and the cmake build-require stop being rebuilt from scratch on every push.
A follow-up commit tightened several of those `timeout-minutes` values down further, from the generous first guesses to numbers closer to what the jobs actually take.

Three unrelated one-line bugs surfaced in the same pass: `IFormatter.hpp`, `PlainFormatter.hpp`, and `PlainFormatter.cpp` closed their namespace with `// antwika::log` instead of the style guide's `// namespace antwika::log`; `time::FakeClock::now()`'s override had dropped `[[nodiscard]]`, unlike `IClock::now()` and `SystemClock::now()`.

## A coverage gate that actually gates

`scripts/check_full_coverage.py` is new: it fails unless a `gcovr --json-summary` report shows exactly 100% across lines, functions, *and* branches, checked independently.
It's wired into `build.yml`'s `apps` job, gated to `matrix.image == 'gnu'`, right after the existing coverage-report step — LLVM stays informational-only, per the project's own documented stance that its branch percentage isn't directly comparable to GNU's.

Turning that gate on immediately found what it was built to find: `World.hpp`'s new `alive()` re-check is a template, so `add<Position>` and `add<Velocity>` are separately compiled, separately branch-tracked copies of the same source line — the existing regression test only drove the "destroyed first" path through `Position`, leaving `Velocity`'s copy of that branch untaken.
Extending the test to stage both types through the same ordering closed it.

## The last branch, taken three times to actually close

One gap survived that fix: `View<Worker>::smallestEntitiesOf` at `View.hpp:83`, called from both `WorkerCompletionSystem.cpp` and `StatusPrintSystem.cpp`, each silently compiling its own copy of `View<Worker>` — genuinely exercised, but with its gcov counters split across two object files that individually looked empty.
An `extern template` looked like the fix, confirmed via `nm` to correctly force one instantiation for a normal build — but a coverage-instrumented build still emits separate, unmerged counters per translation unit for the suppressed copies regardless, so `extern template` doesn't help under `--coverage`.
The real fix was a shared, non-template `antwika::task_worker::allWorkers(world)` function, defined once, that both systems call instead of touching `view<Worker>()` directly — `View<Worker>` is now instantiated in exactly one place.

That still left the same line uncovered from a different direction.
`View<Velocity>` — the single-component instantiation — was only ever constructed with a null storage across the whole suite, so its copy of `smallestEntitiesOf` compiled but its `consider()` lambda never actually ran; a new `WorldTest.ViewOverASingleComponentTypeWithDataReturnsThatEntity` gives it real data to walk.
And even that left `Position`'s own single-component copy short, because under gcov the *same* instantiation is tracked separately per translation unit, and `WorldTest.cpp`'s own `view<Position>()` calls were both against empty worlds — real `Position` coverage only ever came from a different test file entirely.
Extending `WorldTest.ViewIntersectsMultipleComponentTypes` to also check `world.view<Position>()` alone, against data it had already set up, closed it for good: three successive full clean coverage rebuilds confirmed 1339/1339 lines, 312/312 functions, 922/922 branches, and `check_full_coverage.py` passing against the real summary.

## Where it ended up

- Three critical bugs fixed at their source, each with a regression test that reproduces the exact scenario that used to get it wrong: `wfc::Solver` recording partial mutations unconditionally, `ecs::World::add<T>` re-checking `alive()` at apply time, and `antwika::replay` reading untrusted lengths in bounded chunks instead of trusting them up front.
- Five app/library boundaries now reject malformed input instead of silently misbehaving on it, each behind its own new `*Error` type.
- Sixteen classes marked `final`, `libs/event`'s includes rewritten to the path convention the rest of the repo already used, and two more perf/robustness fixes (`Solver`'s reused `queued` vector, `writeString`'s length-prefix bound check).
- `check_unused_test_doubles.py` matches real `#include` paths instead of bare-filename substrings; two other checker scripts gained a shebang-guard fix and pinned UTF-8 decoding.
- CI gained a concurrency group fixing a real badge-publish race, `timeout-minutes` on every job (tightened twice as real run times came in), and Conan/Docker layer caching.
- `scripts/check_full_coverage.py` gates the GNU build on 100% lines/functions/branches; getting there took four separate rounds of chasing what "100%" actually meant across templates, translation units, and per-instantiation gcov counters.
