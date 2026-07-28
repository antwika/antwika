# CHECKLIST: Wave Function Collapse (`antwika::wfc`) and `apps/sudoku`

Companion to `PLAN_WFC.md`. Granular, checkable steps in build order.

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
- [ ] `contains`, `remove`, `restrictTo`, `count`, `isEmpty`,
      `isSingleton`, `singleValue` (precondition-checked), `==`.
- [ ] Ascending iteration (`begin()`/`end()`) over remaining values.
- [ ] `DomainTest.cpp`: construction, mutation, iteration order,
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

## 3. `Solver`

- [ ] `SolveResult.hpp`: `SolveOutcome` (`Solved`/`Unsatisfiable`),
      `SolveResult` (`outcome` + `assignment`).
- [ ] `WfcError.hpp`: one exception type, constructed with a reason
      (mismatched domain alphabet sizes, out-of-range constraint cell
      index) — mirrors `EcsError`/`ReplayFormatError`'s "one specific,
      catchable type" shape, per `PLAN_WFC.md` §3.8.
- [ ] `Solver.hpp/.cpp`: constructor validates wave/constraint
      consistency (throws `WfcError` on violation); `solve()`
      implements propagate-then-collapse per `PLAN_WFC.md` §3.7.
- [ ] Propagation loop: repeatedly runs every constraint's `prune()`,
      in the caller-supplied `std::vector` order, until a full pass
      changes nothing; returns failure immediately on any `false`.
- [ ] Cell selection: lowest candidate-count cell with `count() > 1`,
      ties broken by lowest index (deterministic MRV, no RNG).
- [ ] Candidate order: ascending value order at the chosen cell
      (deterministic, fixed).
- [ ] Backtracking: each candidate is tried via a full-wave copy
      (`PLAN_WFC.md` §3.7's chosen by-value trade-off, not an
      undo-stack); a dead end abandons that copy and tries the next
      candidate; exhausting every candidate at a cell returns
      `Unsatisfiable` from that branch.
- [ ] `SolverPropagationTest.cpp`: a wave solvable by naked singles
      alone reaches `Solved` with propagation only (no branch taken
      needs to backtrack).
- [ ] `SolverBacktrackingTest.cpp`: a crafted wave where propagation
      alone is insufficient — at least one wrong branch is taken and
      abandoned before the correct one is found, and the final
      `SolveResult` is still `Solved` with a valid assignment.
- [ ] `SolverCompletenessTest.cpp`: a crafted unsatisfiable wave
      returns `Unsatisfiable` (proving the search doesn't give up
      early); a crafted solvable wave with a unique solution returns
      exactly that solution.
- [ ] `SolverDeterminismTest.cpp`: the same wave/constraints solved
      twice (two independently constructed `Solver`s or two `solve()`
      calls) produce a bit-identical `SolveResult`.
- [ ] `tests/mocks/include/antwika/wfc/mocks/MockConstraint.hpp`
      created and consumed by at least one `.cpp` test.

## 4. The literal one-dimensional demo

- [ ] `OneDimensionalWfcTest.cpp`: a short 1D sequence of cells (no
      grid, no flattening trick — genuinely 1D), a small symbol
      alphabet, `AdjacencyConstraint` between every consecutive pair
      via a `CompatibilityTable`, per `PLAN_WFC.md` §7.
- [ ] Test asserts every adjacent pair in the resulting assignment is
      compatible per the table, and that solving is deterministic
      (same as `SolverDeterminismTest`, applied to this concrete case).

## 5. Scaffold `apps/sudoku`

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

## 6. `Board`

- [ ] `BoardFormatError.hpp`: one exception type for parse failures
      (wrong length after stripping whitespace, invalid character).
- [ ] `Board.hpp/.cpp`: `parse(text)` (throws `BoardFormatError`),
      `format()`, `at(row, col)`, `set(row, col, digit)`, per
      `PLAN_WFC.md` §5.1.
- [ ] `BoardTest.cpp`: parse/format round-trip for a valid puzzle
      string; `BoardFormatError` for wrong length and for an invalid
      character; both `.` and `0` accepted as the blank marker.

## 7. `Puzzle`

- [ ] `Puzzle.hpp/.cpp`: `buildInitialWave(board)` (givens ->
      singleton `Domain`, blanks -> full `Domain(9)`) and
      `buildConstraints()` (9 rows + 9 columns + 9 boxes as
      `AllDifferentConstraint`s), per `PLAN_WFC.md` §5.2.
- [ ] `PuzzleTest.cpp`: each of the 27 constraints covers exactly the
      right 9 cell indices (spot-check at least one row, one column,
      one box against hand-computed indices); `buildInitialWave`
      produces the right singleton/full domain per cell against a
      hand-built board.

## 8. CLI and integration

- [ ] `main.cpp`: optional `--puzzle <path>` argument; falls back to
      a built-in demo puzzle constant when absent, per `PLAN_WFC.md`
      §5.3. Prints the input board, solves, prints the solved grid or
      a "no solution" message with a non-zero exit code.
- [ ] `SudokuSolverIntegrationTest.cpp`: a known easy puzzle and a
      known hard puzzle each solve to their known expected solution.
- [ ] `UnsolvablePuzzleTest.cpp`: a puzzle with contradictory givens
      (e.g. two identical digits in one row) solves to
      `SolveOutcome::Unsatisfiable`.
- [ ] `SudokuDeterminismTest.cpp`: the same puzzle solved twice (app
      level, via `Puzzle`/`Solver` together) produces identical
      output.

## 9. Cross-cutting / hygiene

- [ ] No line in `src/libs/wfc/**/*.{hpp,cpp}` or
      `src/apps/sudoku/**/*.{hpp,cpp}` exceeds 80 characters
      (`scripts/check_line_length.py` covers `src/**` already).
- [ ] Doxygen `@brief`/`@param`/`@return` on every public class and
      method under `include/antwika/wfc/` and `include/antwika/sudoku/`.
- [ ] No `std::unordered_map`/`unordered_set` (or anything else whose
      iteration order isn't a documented, stable invariant) anywhere
      iteration order could leak into `solve()`'s output — constraints
      and candidate values are iterated only via `std::vector`/`Domain`
      order.
- [ ] No RNG/PRNG anywhere in either the library or the app.
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
- [ ] Once landed, consider whether `REQUIREMENTS.md` should gain
      lines documenting the WFC determinism/completeness guarantee and
      an app that intentionally doesn't use the engine/replay stack
      (`PLAN_WFC.md` §6, deliberately not edited during planning).

## 10. `.github/workflows/` verification (final check before merge)

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

## 11. Write-up (final item)

- [ ] A `blog/` post added (next number in sequence — `blog/004-...`
      is the latest today, so this would be `blog/005-...`) describing
      the design actually implemented: the propagate/collapse/
      backtrack loop, the determinism and completeness guarantees, how
      the same 1D core expresses both a literal tile-adjacency strip
      and Sudoku's row/column/box rules, and any place the real
      implementation deviated from `PLAN_WFC.md`'s sketch (matching
      the precedent set by `blog/001-...` through `blog/004-...`, each
      written after its corresponding work landed, capturing what
      actually happened rather than what was planned).
- [ ] `PLAN_WFC.md` and `PLAN_WFC_CHECKLIST.md` either deleted (if the
      blog post and code fully capture the design, matching the ECS
      precedent's `docs/PLAN.md`/`docs/CHECKLIST.md` removal) or kept
      and closed out with notes on any deviation, at the user's
      preference at that point.
