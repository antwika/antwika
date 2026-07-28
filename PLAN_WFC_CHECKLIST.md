# CHECKLIST: Wave Function Collapse (`antwika::wfc`) and `apps/sudoku`

Companion to `PLAN_WFC.md`. Granular, checkable steps in build order.

Revised after a first review pass: §3 now covers `Trail`/
`EntropyIndex`/weighted entropy/step budget instead of the original
copy-per-branch, full-rescan design — see `PLAN_WFC.md` §3 for why.

## 0. Scaffold `antwika::wfc`

- [ ] Create `src/libs/wfc/{CMakeLists.txt,include,src,tests}`.
- [ ] `include/antwika/wfc/` directory created, empty.
- [ ] `add_library(antwika_wfc ...)` + `antwika::wfc` alias in
      `src/libs/wfc/CMakeLists.txt`, matching the shape of
      `src/libs/time/CMakeLists.txt` (install rules, export set,
      `WINDOWS_EXPORT_ALL_SYMBOLS ON`).
- [ ] No `target_link_libraries` entries at all — `antwika::wfc` has
      zero dependencies on any other library in the repo
      (`PLAN_WFC.md` §2).
- [ ] `add_subdirectory(wfc)` added to `src/libs/CMakeLists.txt`.
- [ ] `antwika_wfc_tests` executable scaffolded in
      `src/libs/wfc/tests/CMakeLists.txt`, registered with
      `gtest_discover_tests`.
- [ ] Project builds with zero `.cpp` files yet (empty lib compiles).

## 1. `Domain`

- [ ] `Domain.hpp`/`.cpp`: bitset-backed candidate set over
      `[0, alphabetSize)`, per `PLAN_WFC.md` §3.2.
- [ ] `Domain(alphabetSize)` starts with every bit set (full domain).
- [ ] `Domain::singleton(value, alphabetSize)` starts with exactly one
      bit set.
- [ ] `contains`, `remove`, `add` (restores a candidate — new versus
      the first draft, needed by `Trail::rewindTo`, §3), `restrictTo`,
      `count`, `isEmpty`, `isSingleton`, `singleValue`
      (precondition-checked), `==`.
- [ ] Ascending iteration (`begin()`/`end()`) over remaining values.
- [ ] `DomainTest.cpp`: construction, mutation (including `add()`
      after `remove()` restoring exactly that bit), iteration order,
      equality, and the `isEmpty`/`isSingleton` boundary cases (zero
      bits, exactly one bit, more than one bit).

## 2. Constraints

- [ ] `IConstraint.hpp`: `cells()` returning
      `std::span<const std::size_t>`, `prune(std::vector<Domain>&)
      const` returning `false` only on contradiction, per `PLAN_WFC.md`
      §3.3.
- [ ] `AllDifferentConstraint.hpp/.cpp`: naked-single elimination
      across all cells it covers, per `PLAN_WFC.md` §3.4.
- [ ] `CompatibilityTable.hpp/.cpp`: square boolean matrix,
      `compatible(a, b) const`, constructed from an explicit
      alphabet size and pairwise compatibility data.
- [ ] `AdjacencyConstraint.hpp/.cpp`: binary arc-consistency between
      two cell indices via a `CompatibilityTable`, per `PLAN_WFC.md`
      §3.5.
- [ ] `AllDifferentConstraintTest.cpp`: a singleton cell's value is
      removed from every other covered cell; an already-contradictory
      input (two covered cells already the same singleton value)
      returns `false` from `prune()`.
- [ ] `AdjacencyConstraintTest.cpp`: incompatible values are pruned
      from both sides; a domain left with no compatible partner
      returns `false` from `prune()`.

## 3. `Trail` and `EntropyIndex` (private internals)

- [ ] `Trail.hpp/.cpp` (private, under `src/`, not `include/`):
      `record(cell, value)`, `checkpoint() const`,
      `rewindTo(checkpoint, wave, entropyIndex)` replaying removals
      after that checkpoint in reverse via `Domain::add()`, per
      `PLAN_WFC.md` §3.9.
- [ ] `TrailTest.cpp`: `rewindTo` restores the exact prior `Domain`
      for every affected cell, including a case where several values
      were removed from the same cell (via multiple `record()` calls
      or one `restrictTo()`) between two checkpoints.
- [ ] `EntropyIndex.hpp/.cpp` (private, under `src/`): `update(cell,
      domain)`, `pickNext() const` returning the lowest-entropy cell
      with `count() > 1` (ties by lowest index), better than `O(n)`
      per call — exact internal structure is an implementation
      detail (`PLAN_WFC.md` §10).
- [ ] `EntropyIndexTest.cpp`: `pickNext()` picks correctly under
      uniform weights (equivalent to plain MRV) and under custom
      weights (`PLAN_WFC.md` §3.7's formula); `update()` keeps the
      index consistent across a shrink-then-restore (rewind)
      sequence, not just monotonically shrinking domains.

## 4. `Solver`

- [ ] `SolveResult.hpp`: `SolveOutcome`
      (`Solved`/`Unsatisfiable`/`LimitExceeded`), `SolveResult`
      (`outcome` + `assignment`), per `PLAN_WFC.md` §3.6.
- [ ] `SolverLimits.hpp`: `SolverLimits` struct with
      `std::optional<std::uint64_t> maxSteps` (default: unlimited),
      per `PLAN_WFC.md` §3.8.
- [ ] `WfcError.hpp`: one exception type, constructed with a reason
      (mismatched domain alphabet sizes, out-of-range constraint cell
      index) — mirrors `EcsError`/`ReplayFormatError`'s "one specific,
      catchable type" shape, per `PLAN_WFC.md` §3.12.
- [ ] `Solver.hpp/.cpp`: constructor takes `initialWave`,
      `constraints`, optional `valueWeights` (default empty ==
      uniform), optional `SolverLimits` (default unlimited); validates
      wave/constraint consistency (throws `WfcError` on violation);
      builds `cellToConstraints` once, per `PLAN_WFC.md` §3.9.
- [ ] Propagation: worklist-driven, not repeated full passes over
      every constraint — the initial `propagate()` call seeds the
      worklist with every constraint once; every later call (after one
      collapse) seeds it with only the constraints touching the
      collapsed cell, per `PLAN_WFC.md` §3.9's pseudocode.
- [ ] Cell selection: via `EntropyIndex::pickNext()`, not a linear
      rescan of the wave (deterministic MRV/weighted-entropy, no RNG).
- [ ] Candidate order: ascending value order at the chosen cell
      (deterministic, fixed).
- [ ] Backtracking: iterative, an explicit choice-point stack (not
      recursion — search depth must never be bounded by the C++ call
      stack, per `PLAN_WFC.md` §1's scalability goal); `solve()` copies
      `initialWave` exactly once, never once per branch; each branch's
      effects are undone via `Trail::rewindTo`, not a fresh wave copy.
- [ ] Step budget: one step per candidate value attempted at a choice
      point (not per propagation call); reaching `maxSteps` returns
      `SolveOutcome::LimitExceeded` immediately, distinct from
      `Unsatisfiable`, per `PLAN_WFC.md` §3.8/§3.11.
- [ ] `SolverPropagationTest.cpp`: a wave solvable by naked singles
      alone reaches `Solved` with propagation only (no branch taken
      needs to backtrack).
- [ ] `SolverBacktrackingTest.cpp`: a crafted wave where propagation
      alone is insufficient — at least one wrong branch is taken and
      abandoned before the correct one is found, and the final
      `SolveResult` is still `Solved` with a valid assignment.
- [ ] `SolverCompletenessTest.cpp`: a crafted unsatisfiable wave (with
      unlimited `maxSteps`) returns `Unsatisfiable` (proving the
      search doesn't give up early); a crafted solvable wave with a
      unique solution returns exactly that solution.
- [ ] `SolverDeterminismTest.cpp`: the same wave/constraints/weights/
      limits solved twice (two independently constructed `Solver`s or
      two `solve()` calls) produce a bit-identical `SolveResult`.
- [ ] `WeightedEntropyTest.cpp`: custom `valueWeights` change which
      cell collapses first versus plain MRV, for a crafted wave with
      two equal-candidate-count cells and differing weight
      distributions; omitting `valueWeights` on that same wave
      reproduces the unweighted MRV collapse order exactly.
- [ ] `SolverStepLimitTest.cpp`: a tiny `maxSteps` on a search needing
      many candidates returns `LimitExceeded`, never
      `Unsatisfiable` or `Solved`; a generous `maxSteps` still lets an
      otherwise-normal small solve finish as `Solved`.
- [ ] `tests/mocks/include/antwika/wfc/mocks/MockConstraint.hpp`
      created and consumed by at least one `.cpp` test.

## 5. Scale and the literal one-dimensional demo

- [ ] `OneDimensionalWfcTest.cpp`: a short 1D sequence of cells (no
      grid, no flattening trick — genuinely 1D), a small symbol
      alphabet, `AdjacencyConstraint` between every consecutive pair
      via a `CompatibilityTable`, per `PLAN_WFC.md` §7.
- [ ] Test asserts every adjacent pair in the resulting assignment is
      compatible per the table, and that solving is deterministic
      (same as `SolverDeterminismTest`, applied to this concrete case).
- [ ] `SolverLargeScaleTest.cpp`: a few-thousand-cell 1D wave with
      simple, mostly-satisfiable `AdjacencyConstraint`s completes with
      a valid assignment (every constraint holds in the result);
      asserts completion within the test framework's default timeout,
      not a specific wall-clock target — a scale regression check for
      `PLAN_WFC.md` §3.9's design, not a timed benchmark.

## 6. Scaffold `apps/sudoku`

- [ ] Create `src/apps/sudoku/{CMakeLists.txt,include,src,tests}`.
- [ ] `add_executable(antwika_sudoku ...)` in
      `src/apps/sudoku/CMakeLists.txt`, matching the shape of
      `src/apps/life/CMakeLists.txt` (include dirs, MinGW DLL-copy
      post-build step, install rule).
- [ ] `target_link_libraries(antwika_sudoku PRIVATE antwika::wfc)` —
      no `antwika::engine`/`replay`/`time`/`log` (`PLAN_WFC.md` §5.5).
- [ ] `add_subdirectory(sudoku)` added to `src/apps/CMakeLists.txt`.
- [ ] `antwika_sudoku_tests` executable scaffolded in
      `src/apps/sudoku/tests/CMakeLists.txt`.

## 7. `Board`

- [ ] `BoardFormatError.hpp`: one exception type for parse failures
      (wrong length after stripping whitespace, invalid character).
- [ ] `Board.hpp/.cpp`: `parse(text)` (throws `BoardFormatError`),
      `format()`, `at(row, col)`, `set(row, col, digit)`, per
      `PLAN_WFC.md` §5.1.
- [ ] `BoardTest.cpp`: parse/format round-trip for a valid puzzle
      string; `BoardFormatError` for wrong length and for an invalid
      character; both `.` and `0` accepted as the blank marker.

## 8. `Puzzle`

- [ ] `Puzzle.hpp/.cpp`: `buildInitialWave(board)` (givens ->
      singleton `Domain`, blanks -> full `Domain(9)`) and
      `buildConstraints()` (9 rows + 9 columns + 9 boxes as
      `AllDifferentConstraint`s), per `PLAN_WFC.md` §5.2.
- [ ] `PuzzleTest.cpp`: each of the 27 constraints covers exactly the
      right 9 cell indices (spot-check at least one row, one column,
      one box against hand-computed indices); `buildInitialWave`
      produces the right singleton/full domain per cell against a
      hand-built board.

## 9. CLI and integration

- [ ] `main.cpp`: optional `--puzzle <path>` argument; falls back to
      a built-in demo puzzle constant when absent, per `PLAN_WFC.md`
      §5.3. Constructs `Solver` with only the wave and constraints —
      no `valueWeights`, no `SolverLimits` (Sudoku needs neither).
      Prints the input board, solves, prints the solved grid or a
      "no solution" message with a non-zero exit code; handles
      `SolveOutcome::LimitExceeded` explicitly too, even though it
      cannot occur here, so the switch stays exhaustive.
- [ ] `SudokuSolverIntegrationTest.cpp`: a known easy puzzle and a
      known hard puzzle each solve to their known expected solution.
- [ ] `UnsolvablePuzzleTest.cpp`: a puzzle with contradictory givens
      (e.g. two identical digits in one row) solves to
      `SolveOutcome::Unsatisfiable`.
- [ ] `SudokuDeterminismTest.cpp`: the same puzzle solved twice (app
      level, via `Puzzle`/`Solver` together) produces identical
      output.

## 10. Cross-cutting / hygiene

- [ ] No line in `src/libs/wfc/**/*.{hpp,cpp}` or
      `src/apps/sudoku/**/*.{hpp,cpp}` exceeds 80 characters
      (`scripts/check_line_length.py` covers `src/**` already).
- [ ] Doxygen `@brief`/`@param`/`@return` on every public class and
      method under `include/antwika/wfc/` and `include/antwika/sudoku/`
      (`Trail`/`EntropyIndex` are private, under `src/`, and exempt
      from the public-surface Doxygen rule, same as `EntityManager`).
- [ ] No `std::unordered_map`/`unordered_set` (or anything else whose
      iteration order isn't a documented, stable invariant) anywhere
      iteration order could leak into `solve()`'s output — constraints
      and candidate values are iterated only via `std::vector`/`Domain`
      order; `EntropyIndex`'s internal structure (§3) must be checked
      against this same rule once its concrete type is chosen.
- [ ] No RNG/PRNG anywhere in either the library or the app.
- [ ] No wall-clock/real-time-based timeout anywhere — `SolverLimits`
      is step-counted only, per explicit instruction (`PLAN_WFC.md`
      §3.8).
- [ ] `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on
      GNU and LLVM toolchains.
- [ ] `README.md`'s project-structure listing gains `wfc/` under
      `libs/` and `sudoku/` under `apps/`, plus a short section (like
      the existing "Replays" section) describing how to run
      `antwika_sudoku`.
- [ ] Coverage: GNU/LLVM builds pass with the new tests included; any
      `GCOVR_EXCL_LINE` is justified by a comment and added only after
      confirming a real, unreachable gap, per
      `docs/confirming-unreachable-branches.md`.
- [ ] Once landed, cross-check the current implementation against the
      candidate `REQUIREMENTS.md` additions recorded in `ISSUES.md`
      and decide which, if any, to actually add.

## 11. `.github/workflows/` verification (final check before merge)

- [ ] `.github/workflows/build.yml`'s non-MinGW "Verify executables"
      `expected` array includes `antwika_wfc_tests`, `antwika_sudoku`,
      and `antwika_sudoku_tests`.
- [ ] `.github/workflows/build.yml`'s MinGW `expected` array includes
      `antwika_sudoku` alongside the existing `antwika_game` and
      `antwika_life` (MinGW builds no `tests/` subdirectories at all,
      per `CMAKE_CROSSCOMPILING`, so no `*_tests` entries belong here).
- [ ] `.github/workflows/build.yml`'s coverage `gcovr --exclude` list
      includes `.*/apps/sudoku/src/main\.cpp`, matching the existing
      `apps/game`/`apps/life` `main.cpp` exclusions.
- [ ] `.github/workflows/ci.yml` and `.github/workflows/release.yml`
      re-checked and confirmed to need **no** changes (neither
      references specific libraries or apps by name).
- [ ] A CI run (or a local dry run of the same steps) confirms every
      newly expected binary actually exists after a build, on at
      least the GNU toolchain.

## 12. Write-up (final item)

- [ ] A `blog/` post added (next number in sequence — `blog/004-...`
      is the latest today, so this would be `blog/005-...`) describing
      the design actually implemented: the worklist propagation, trail-
      based backtracking, and incremental entropy index behind the
      propagate/collapse/backtrack loop; the determinism and
      completeness guarantees (and how the step budget's
      `LimitExceeded` outcome avoids weakening completeness); weighted
      entropy as an opt-in generalization; how the same 1D core
      expresses both a literal tile-adjacency strip and Sudoku's
      row/column/box rules; and any place the real implementation
      deviated from `PLAN_WFC.md`'s sketch (matching the precedent set
      by `blog/001-...` through `blog/004-...`, each written after its
      corresponding work landed, capturing what actually happened
      rather than what was planned).
- [ ] `PLAN_WFC.md` and `PLAN_WFC_CHECKLIST.md` deleted — per explicit
      instruction, these are guidance for the implementation only and
      are removed once it and the blog post above are both done,
      matching the ECS precedent's `docs/PLAN.md`/`docs/CHECKLIST.md`
      removal (not left open to preference, unlike that precedent).
