# PLAN: Wave Function Collapse (`antwika::wfc`) and `apps/sudoku`

Planning doc, written before any code lands, per the same
`docs/PLAN.md`/`docs/CHECKLIST.md` precedent used for `antwika::ecs`.
Named `PLAN_WFC.md`/`PLAN_WFC_CHECKLIST.md` (root-level, topic-suffixed)
rather than reusing the generic `docs/PLAN.md` name, so a future
per-feature plan can coexist without a collision — see `ISSUES.md` for
this and a few other judgment calls made while writing this plan.

## 1. Goal

Add a generic, reusable constraint-solving library at `src/libs/wfc`
(`antwika::wfc`), following the exact convention every existing
library already uses: its own `CMakeLists.txt`, `include/`, `src/`,
and `tests/` directory under `src/libs/<name>`.

The library implements Wave Function Collapse (WFC): repeatedly pick
the lowest-entropy cell, collapse it to one candidate value, propagate
the consequences to every other cell, and backtrack on contradiction.
Three properties are required, all achievable within that same loop:

- **Deterministic** — no RNG anywhere; every tie (which cell to
  collapse next, which candidate value to try first) is broken by a
  fixed, documented rule, so the same input always produces the same
  output.
- **Complete** — if a solution exists, the algorithm finds it. Classic
  WFC implementations are randomized and allowed to fail on
  contradiction (typically by restarting with a new seed); this one
  instead backtracks exhaustively, so failure only ever means "proven
  unsatisfiable," never "got unlucky."
- **One dimensional** — the algorithm's own data model is a flat,
  index-addressed `std::vector` of cells. It has no concept of a grid,
  rows, columns, or neighbors. All spatial/relational structure is
  supplied by the caller as constraints over cell indices. See §3.1
  for why this is what makes both a classic 1D WFC use case *and* a
  2D puzzle like Sudoku expressible on top of the same core, and
  `ISSUES.md` for the ambiguity this resolves.

The showcase application, `src/apps/sudoku` (`antwika::sudoku` /
binary `antwika_sudoku`), solves a Sudoku puzzle by expressing its
givens as an initial 81-cell wave and its rows/columns/3x3-box rules
as constraints over that flat array, then hands both to
`antwika::wfc::Solver` — no 2D-grid code inside the library at all.

## 2. Non-goals

- No built-in grid/adjacency geometry (2D, hex, or otherwise). The
  library only ever sees `std::vector<Domain>` plus a list of
  constraints over indices into it — geometry is entirely an
  application concern, expressed by which indices a constraint
  touches, the same way `apps/life`'s `Grid` maps `(x, y)` to `Entity`
  entirely outside `antwika::ecs`.
- No tile-frequency/weighted-entropy model. Classic WFC often biases
  cell selection and value choice by authored tile weights, to make
  generated output look more "natural." Sudoku has no such notion, and
  determinism only requires *some* fixed rule, not a weighted one, so
  entropy here is simply "count of remaining candidate values" (the
  standard CSP "minimum remaining values" heuristic) — a deliberate
  simplification, not an oversight. A weighted variant is a possible
  future extension, not required by anything asked for here.
- No RNG/PRNG anywhere, matching the project-wide "Won't have."
- No general SAT/SMT solving. Two constraint kinds ship
  (`AllDifferentConstraint`, `AdjacencyConstraint`, §3.4–3.5) — enough
  for classic tile-adjacency WFC and for Sudoku's row/column/box
  rules. The `IConstraint` interface is the extension point for any
  future kind; inventing more than these two isn't needed today.
- No incremental/step-by-step solving API for visualization. `solve()`
  is one-shot: given a wave and constraints, it returns a result. A
  future step-by-step observer hook is a documented possible
  extension, not required by the Sudoku showcase.
- No multithreaded search.
- `antwika::wfc` will not depend on `antwika::engine`, `antwika::event`,
  `antwika::replay`, `antwika::time`, or `antwika::log`. It is a pure,
  self-contained algorithm library — even more standalone than
  `antwika::ecs`, which still depends on `antwika::log` for one fatal
  path. Nothing in the solver needs logging, clocks, or events.
- `apps/sudoku` will not integrate with `antwika::engine`/`replay`/the
  fixed-timestep tick loop the way `apps/game` and `apps/life` do.
  Solving a puzzle is one batch computation, not an ongoing
  simulation with external input over time — there is no tick to
  drive and nothing to replay. See §5.5 and `ISSUES.md` for this
  deliberate deviation from the existing two apps' shape.

## 3. Core design (`antwika::wfc`)

### 3.1 Why "one dimensional" and "solves Sudoku" aren't in tension

Sudoku is visually a 9x9 grid, but nothing about solving it actually
requires the *solver* to know that. A Sudoku constraint ("these nine
cells must all hold different values") is just a set of indices into
a flat 81-element array — row 3 is indices `27..35`, column 3 is
indices `{3, 12, 21, ..., 75}`, and each 3x3 box is nine more indices
computed from `(row / 3, col / 3)`. Once that index math happens once,
in `apps/sudoku` (§5.2), the solver never sees a row, a column, or a
box again — only `std::vector<Domain>` and a list of index sets.

That is the intended reading of "must be one dimensional": the core
algorithm's storage and API are 1D and geometry-agnostic, not that
every *problem* solved with it must itself be a literal 1D sequence.
The library's own tests still include a genuinely 1D use case (a
classic tile-adjacency WFC strip, §7) precisely so "one dimensional"
is demonstrated directly, not only inferred from Sudoku's index math.

### 3.2 `Domain`

```cpp
class Domain
{
public:
    explicit Domain(std::size_t alphabetSize);      // all bits set
    static Domain singleton(std::size_t value,
                             std::size_t alphabetSize);

    [[nodiscard]] bool contains(std::size_t value) const;
    void remove(std::size_t value);
    void restrictTo(std::size_t value);

    [[nodiscard]] std::size_t count() const;
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] bool isSingleton() const;
    [[nodiscard]] std::size_t singleValue() const;  // pre: isSingleton()

    // Ascending iteration over remaining candidate values.
    const_iterator begin() const;
    const_iterator end() const;

    bool operator==(const Domain &) const = default;

private:
    std::vector<bool> bits;
};
```

A small bitset-backed value type: the set of candidate symbol indices
`[0, alphabetSize)` still allowed for one cell. `std::vector<bool>`
(not a fixed-width `std::bitset<N>`) so the same type serves a 3-tile
WFC strip and Sudoku's 9-symbol alphabet without a template parameter.

### 3.3 `IConstraint`

```cpp
class IConstraint
{
public:
    virtual ~IConstraint() = default;

    // The cell indices this constraint reads/prunes.
    virtual std::span<const std::size_t> cells() const = 0;

    // Prune wave[cells()] in place. Returns false iff a domain became
    // empty (a contradiction) -- true otherwise, including "no change."
    virtual bool prune(std::vector<Domain> &wave) const = 0;
};
```

One small interface, same shape as `ISystem`/`IEventSink` elsewhere —
a constraint is anything that can look at (and narrow) the domains of
the cells it cares about. `prune()` doubles as the *only* consistency
check the solver needs (§3.7): collapsing a cell to a value and then
re-running `prune()` on every constraint touching it is what both
propagates the consequence and detects a violation, with no separate
"is this assignment legal" method required.

### 3.4 `AllDifferentConstraint`

```cpp
class AllDifferentConstraint final : public IConstraint
{
public:
    explicit AllDifferentConstraint(std::vector<std::size_t> cellIndices);
    std::span<const std::size_t> cells() const override;
    bool prune(std::vector<Domain> &wave) const override;

private:
    std::vector<std::size_t> cellIndices;
};
```

`prune()` applies the standard "naked single" rule: for every cell in
`cellIndices` that is currently a singleton with value `v`, remove `v`
from every *other* cell in `cellIndices`. This is what Sudoku's rows,
columns, and boxes are built from (§5.2) — it is not full
hyper-arc-consistency for all-different (it won't spot every possible
inconsistency early), and it doesn't need to: completeness comes from
the backtracking search in §3.7, not from how aggressively propagation
prunes. Propagation is purely a speed optimization on top of a search
that is correct even with the weakest possible `prune()` (one that
always returns `true` and changes nothing).

### 3.5 `AdjacencyConstraint`

```cpp
class AdjacencyConstraint final : public IConstraint
{
public:
    AdjacencyConstraint(std::size_t left, std::size_t right,
                         CompatibilityTable table);
    std::span<const std::size_t> cells() const override;  // {left, right}
    bool prune(std::vector<Domain> &wave) const override;

private:
    std::size_t left;
    std::size_t right;
    CompatibilityTable table;
};
```

`CompatibilityTable` is a small square boolean-matrix value type:
`compatible(a, b)` says whether symbol `a` at `left` may sit next to
symbol `b` at `right`. `prune()` is standard binary arc-consistency
(AC-3's arc revision): remove any value from `left`'s domain that has
no compatible value left in `right`'s domain, and symmetrically for
`right`. This is the classic WFC "tile adjacency" rule, expressed as
a constraint between two flat indices — it is what the library's own
1D demo (§7) uses, and is not needed by `apps/sudoku`, which only uses
`AllDifferentConstraint`. Shipping both from day one is what proves
the interface in §3.3 supports more than one constraint shape.

### 3.6 `SolveResult`

```cpp
enum class SolveOutcome
{
    Solved,
    Unsatisfiable,
};

struct SolveResult
{
    SolveOutcome outcome;
    std::vector<std::size_t> assignment;  // valid iff outcome == Solved
};
```

### 3.7 `Solver`

```cpp
class Solver
{
public:
    Solver(std::vector<Domain> initialWave,
           std::vector<std::reference_wrapper<const IConstraint>>
               constraints);

    [[nodiscard]] SolveResult solve() const;

private:
    std::vector<Domain> wave;
    std::vector<std::reference_wrapper<const IConstraint>> constraints;
};
```

Constructor validates that every `Domain` in `initialWave` shares the
same alphabet size, and that every constraint's `cells()` indices are
in range, throwing `WfcError` otherwise (§3.10) — a cheap, purely
defensive check at the boundary, not a hot-path cost.

`solve()`, in pseudocode:

```
propagate(wave):
    repeat until no constraint's prune() changes anything in a full pass:
        for each constraint, in the order the caller supplied it:
            if not constraint.prune(wave): return false  // contradiction
    return true

collapse(wave):                      // wave taken and returned by value
    if not propagate(wave): return Unsatisfiable

    cell = the lowest-index cell whose domain.count() is the smallest
           value > 1 across the whole wave                 // MRV, §2
    if no such cell exists: return Solved, extract(wave)    // all singleton

    for value in wave[cell], in ascending order:            // fixed order
        branch = wave                                       // copy
        branch[cell] = Domain::singleton(value, alphabetSize)
        result = collapse(branch)
        if result.outcome == Solved: return result

    return Unsatisfiable   // every candidate at `cell` led to a dead end

solve(): return collapse(wave)
```

**Determinism** (§8's tests hold this to account): both the cell
picked at each step (lowest remaining-candidate count, ties broken by
lowest index) and the order candidate values are tried (ascending) are
fixed rules with no randomness and no dependence on container
iteration order beyond a plain `std::vector`'s index order. Running
`solve()` twice on the same wave/constraints always returns the exact
same `SolveResult`.

**Completeness**: every reachable branch at every decision point is
tried, in order, before the search reports `Unsatisfiable` — the
search only gives up on a cell once every one of its remaining
candidate values has been tried and led to a dead end further down.
This is what guarantees "finds a solution if one exists," at the cost
of worst-case exponential time on adversarial inputs (§9, `ISSUES.md`).

**Chosen trade-off**: `collapse()` takes and returns `wave` by value,
copying the whole wave per branch rather than mutating one shared wave
with an undo stack. Simpler to write and to reason about correctly,
at the cost of extra copies; §9 leaves an undo-stack version as a
possible future optimization if a much larger board ever needs it —
not required for Sudoku's 81 cells or a small WFC demo strip.

### 3.8 `WfcError`

One specific, catchable error type — mirrors `EcsError` and
`ReplayFormatError`'s existing "one exception type per library"
precedent — thrown only by `Solver`'s constructor, for mismatched
`Domain` alphabet sizes across the wave or an out-of-range constraint
cell index. Never thrown by `solve()` itself: an unsatisfiable puzzle
is a normal, non-exceptional `SolveResult`, not an error.

## 4. File layout (`src/libs/wfc`)

```
src/libs/wfc/
├── CMakeLists.txt
├── include/antwika/wfc/
│   ├── Domain.hpp
│   ├── IConstraint.hpp
│   ├── AllDifferentConstraint.hpp
│   ├── CompatibilityTable.hpp
│   ├── AdjacencyConstraint.hpp
│   ├── SolveResult.hpp        // SolveOutcome, SolveResult
│   ├── WfcError.hpp
│   └── Solver.hpp
├── src/
│   ├── Domain.cpp
│   ├── AllDifferentConstraint.cpp
│   ├── CompatibilityTable.cpp
│   ├── AdjacencyConstraint.cpp
│   └── Solver.cpp
└── tests/
    ├── CMakeLists.txt
    ├── DomainTest.cpp
    ├── AllDifferentConstraintTest.cpp
    ├── AdjacencyConstraintTest.cpp
    ├── SolverPropagationTest.cpp     // prune-only behavior, no search
    ├── SolverBacktrackingTest.cpp    // contradictions force backtrack
    ├── SolverCompletenessTest.cpp    // crafted solvable/unsatisfiable
    │                                  // CSPs, proves both outcomes
    ├── SolverDeterminismTest.cpp     // same input solved twice ->
    │                                  // bit-identical SolveResult
    ├── OneDimensionalWfcTest.cpp     // classic tile-adjacency strip,
    │                                  // see §7 -- the literal 1D case
    └── mocks/
        └── include/antwika/wfc/mocks/MockConstraint.hpp
```

`CompatibilityTable` gets its own header/`.cpp` rather than living
inside `AdjacencyConstraint.hpp` since it is a small, independently
testable value type (a square boolean matrix), the same granularity
`antwika::ecs` uses for e.g. `Entity`/`EcsError` as separate headers.

## 5. Showcase application (`src/apps/sudoku`)

### 5.1 `Board`

```cpp
class Board
{
public:
    static constexpr std::size_t kSize = 9;
    static constexpr std::size_t kCellCount = kSize * kSize;

    static Board parse(std::string_view text);  // throws BoardFormatError
    [[nodiscard]] std::string format() const;

    [[nodiscard]] std::optional<int> at(std::size_t row,
                                         std::size_t col) const;
    void set(std::size_t row, std::size_t col, int digit);

private:
    std::array<int, kCellCount> cells{};  // 0 == blank, else 1..9
};
```

`parse()` reads the common flat-string Sudoku format: exactly 81
characters once whitespace/newlines are stripped, each one a digit
`1`-`9` or a blank marker (`.` or `0`); anything else — wrong length,
an invalid character — raises `BoardFormatError`, one specific,
catchable type, mirroring `WfcError`/`EcsError`/`ReplayFormatError`.

### 5.2 `Puzzle`: `Board` to `Solver` inputs

```cpp
namespace antwika::sudoku
{
    std::vector<antwika::wfc::Domain> buildInitialWave(const Board &board);

    // 9 rows + 9 columns + 9 boxes = 27 constraints.
    std::vector<AllDifferentConstraint> buildConstraints();
}
```

`buildInitialWave`: for each of the 81 cells, a given digit `d` becomes
`Domain::singleton(d - 1, 9)`; a blank becomes a full `Domain(9)`
(alphabet indices `0..8` represent digits `1..9`). `buildConstraints`:
row `r`'s nine indices are `r*9 .. r*9+8`; column `c`'s are
`{c, c+9, ..., c+72}`; box `b`'s nine indices come from
`(3*(b/3) + dr)*9 + (3*(b%3) + dc)` for `dr, dc` in `0..2` — ordinary
index arithmetic, entirely inside `apps/sudoku`, never inside
`antwika::wfc`.

### 5.3 CLI (`main.cpp`)

Same shape as `apps/game`/`apps/life`'s `main.cpp`: parse `argv` for an
optional `--puzzle <path>`; if absent, fall back to a small, built-in
demo puzzle constant (an easy, well-known example), the same
`--record`-less "baked-in demo, or load one" pattern `demoScript()`
uses in `apps/game`. Print the input board, run `Solver::solve()`,
then print either the solved grid (`SolveOutcome::Solved`) or a
"no solution" message with a non-zero exit code
(`SolveOutcome::Unsatisfiable`).

### 5.4 File layout (`src/apps/sudoku`)

```
src/apps/sudoku/
├── CMakeLists.txt
├── include/antwika/sudoku/
│   ├── Board.hpp
│   ├── BoardFormatError.hpp
│   └── Puzzle.hpp
├── src/
│   ├── Board.cpp
│   ├── Puzzle.cpp
│   └── main.cpp
└── tests/
    ├── CMakeLists.txt
    ├── BoardTest.cpp                     // parse/format round-trip,
    │                                       // BoardFormatError cases
    ├── PuzzleTest.cpp                    // wave + all 27 constraints
    │                                       // reference the right cells
    ├── SudokuSolverIntegrationTest.cpp   // solves known easy + hard
    │                                       // puzzles, matches expected
    │                                       // solutions
    ├── UnsolvablePuzzleTest.cpp          // contradictory givens ->
    │                                       // Unsatisfiable
    └── SudokuDeterminismTest.cpp         // same puzzle solved twice ->
                                            // identical output
```

`antwika_sudoku` links only `antwika::wfc` — no `antwika::engine`,
`antwika::replay`, `antwika::time`, or `antwika::log` (§5.5).

### 5.5 Why `apps/sudoku` skips the engine/replay stack

`apps/game` and `apps/life` both exist to demonstrate the replay
system: a fixed-timestep tick loop consuming external input over time,
recordable and replayable. Solving a Sudoku puzzle has none of that
shape — there is no tick, no ongoing input stream, nothing to record
or replay, just "given this puzzle, compute its solution." Forcing a
one-shot batch computation through machinery built for ongoing
simulated worlds would add ceremony (a fake single tick, a
manufactured event) without demonstrating any engine/replay behavior
that `apps/game`/`apps/life` don't already cover. This is a deliberate
scope decision, not an oversight — flagged in `ISSUES.md` since it is
a real deviation from the shape of the two existing apps, worth the
user's explicit sign-off before implementation starts.

## 6. CI / workflow impact

`.github/workflows/build.yml`'s "Verify executables" step (§8, step
9) needs updated expected-binary lists once `antwika::wfc` and
`antwika::sudoku` exist:

- Non-MinGW `expected`: add `antwika_wfc_tests`, `antwika_sudoku`, and
  `antwika_sudoku_tests`.
- MinGW `expected` (already a shorter, apps-only list, since
  `if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)` skips building
  every library's/app's `tests/` subdirectory when cross-compiling):
  add `antwika_sudoku` alongside the existing `antwika_game` and
  `antwika_life`.
- The coverage step's `gcovr --exclude` list already excludes
  `apps/game/src/main.cpp` and `apps/life/src/main.cpp` (CLI
  arg-parsing/I/O glue, not unit-tested directly); add
  `--exclude '.*/apps/sudoku/src/main\.cpp'` for the same reason.

`ci.yml` and `release.yml` need no changes — neither references
specific libraries/apps by name. `README.md`'s project-structure
listing gains `wfc/` under `libs/` and `sudoku/` under `apps/`, same
as every prior library/app addition.

`REQUIREMENTS.md` is deliberately **not** edited as part of this plan.
It documents already-built, current state (per its own header) and
the ECS precedent (`docs/PLAN.md` §9, at the time) left it unedited
during planning for the same reason — once `antwika::wfc`/
`apps/sudoku` actually land, revisit whether it should gain a line
about the determinism/completeness guarantee and about an app that
doesn't use the replay stack.

## 7. The literal one-dimensional demo

Separately from Sudoku (which proves the core's *generality* — an
arbitrary constraint graph, not just neighbor pairs), the library's
own test suite includes `OneDimensionalWfcTest.cpp`: a short sequence
of cells (e.g. 8-12), each with a 3-4 symbol alphabet (say `grass`,
`sand`, `water`), wired with `AdjacencyConstraint` between every
consecutive pair via a small `CompatibilityTable` (e.g. `water` may
sit next to `sand` or `water`, `sand` next to anything, `grass` never
directly next to `water`). This is classic 1D WFC with no
generalization at all — proving the "one dimensional" requirement is
met literally, not only via Sudoku's flattened indexing.

## 8. Testing strategy

- GoogleTest + CTest, one behavior tested alongside the code that
  introduces it, per the project's existing rule.
- `DomainTest`: bit operations, singleton/empty construction,
  iteration order (ascending), equality.
- `AllDifferentConstraintTest` / `AdjacencyConstraintTest`: pruning
  behavior in isolation, including the "already contradictory input"
  case returning `false`.
- `SolverPropagationTest`: propagation alone solves a fully
  determined-by-naked-singles case with zero backtracking (an "easy"
  Sudoku-shaped CSP is the natural fixture here).
- `SolverBacktrackingTest`: a small crafted CSP where propagation
  alone is insufficient and at least one wrong branch must be tried
  and abandoned before the correct one is found.
- `SolverCompletenessTest`: a crafted unsatisfiable CSP returns
  `Unsatisfiable` only after every branch has genuinely been
  exhausted (assert on outcome, not on internal call counts).
- `SolverDeterminismTest`: same wave/constraints solved twice (or via
  two independently constructed `Solver` instances) yields a
  bit-identical `SolveResult`.
- `OneDimensionalWfcTest`: §7's tile strip, asserting every adjacent
  pair in the final assignment is compatible per the table.
- `apps/sudoku` tests per §5.4: parsing, constraint-set construction,
  solving known easy/hard puzzles against known expected solutions, an
  unsatisfiable puzzle, and a determinism check at the app level too.
- Every mock under `tests/mocks/include` (here, `MockConstraint`) must
  be used by at least one `.cpp`, per the project-wide rule already
  enforced by `scripts/check_unused_test_doubles.py`.

## 9. Step-by-step implementation order

See `PLAN_WFC_CHECKLIST.md` for the granular, checkable version. High
level:

1. `antwika::wfc` scaffold (`CMakeLists.txt`, empty `include/src/tests`,
   wired into `src/libs/CMakeLists.txt`, no library dependencies).
2. `Domain` (+ tests).
3. `IConstraint`, `AllDifferentConstraint`, `CompatibilityTable`,
   `AdjacencyConstraint` (+ tests for each).
4. `SolveResult`, `WfcError`, `Solver` (propagation, then collapse/
   backtracking) (+ propagation/backtracking/completeness/determinism
   tests).
5. `OneDimensionalWfcTest` — the literal 1D demo (§7).
6. `antwika::sudoku` scaffold (`CMakeLists.txt`, wired into
   `src/apps/CMakeLists.txt`, linking only `antwika::wfc`).
7. `Board` + `BoardFormatError` (+ tests).
8. `Puzzle` (initial wave + 27 constraints) (+ tests).
9. `main.cpp` CLI, `.github/workflows/build.yml` expected-binaries and
   `gcovr --exclude` updates (§6), `README.md` project-structure
   update.
10. Integration tests: known easy/hard puzzles, an unsatisfiable
    puzzle, determinism.
11. Final review pass: coverage, workflow verification, blog post —
    see `PLAN_WFC_CHECKLIST.md`'s closing items.

## 10. Open questions

- Should `Solver` eventually support an injected step limit/timeout
  for adversarial inputs (§2, `ISSUES.md`)? Not required by Sudoku or
  the 1D demo; left open for whenever `antwika::wfc` gets used with
  untrusted input.
- Is the copy-per-branch `collapse()` (§3.7) fast enough in practice
  for every puzzle difficulty the showcase wants to demonstrate, or
  does an undo-stack version become necessary? Expected to be a
  non-issue at Sudoku's 81-cell scale; revisit only if profiling says
  otherwise.
- Whether a weighted-entropy variant (§2) is ever worth adding is left
  entirely open — nothing asked for here needs it.
- Whether `REQUIREMENTS.md` gains new lines once this lands, and what
  they should say, is deliberately left for after implementation (§6).
