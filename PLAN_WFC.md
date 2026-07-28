# PLAN: Wave Function Collapse (`antwika::wfc`) and `apps/sudoku`

Planning doc, written before any code lands, per the same
`docs/PLAN.md`/`docs/CHECKLIST.md` precedent used for `antwika::ecs`.
Named `PLAN_WFC.md`/`PLAN_WFC_CHECKLIST.md` (root-level, topic-suffixed)
rather than reusing the generic `docs/PLAN.md` name, so a future
per-feature plan can coexist without a collision — see `ISSUES.md` for
this and a few other judgment calls made while writing this plan.

Revised after a first review pass: the original sketch's copy-per-
branch backtracking and linear entropy scan were sized for an 81-cell
Sudoku board. Scale is now a first-class design goal (large procedural
generation, not just small puzzles), which changed §3.7-3.9
substantially — see the note at the top of §3 for what changed and
why.

**Temporary**: this file and `PLAN_WFC_CHECKLIST.md` exist only to
guide the implementation. Per explicit instruction, both are deleted
once the implementation and its `blog/` write-up (checklist §12) are
complete — the same disposition the equivalent `docs/PLAN.md`/
`docs/CHECKLIST.md` had for `antwika::ecs`, just decided up front here
rather than left open.

## 1. Goal

Add a generic, reusable constraint-solving library at `src/libs/wfc`
(`antwika::wfc`), following the exact convention every existing
library already uses: its own `CMakeLists.txt`, `include/`, `src/`,
and `tests/` directory under `src/libs/<name>`.

The library implements Wave Function Collapse (WFC): repeatedly pick
the lowest-entropy cell, collapse it to one candidate value, propagate
the consequences to every other cell, and backtrack on contradiction.
Four properties are required, all achievable within that same loop:

- **Deterministic** — no RNG anywhere; every tie (which cell to
  collapse next, which candidate value to try first) is broken by a
  fixed, documented rule, so the same input always produces the same
  output.
- **Complete** — if a solution exists, the algorithm finds it. Classic
  WFC implementations are randomized and allowed to fail on
  contradiction (typically by restarting with a new seed); this one
  instead backtracks exhaustively, so failure only ever means "proven
  unsatisfiable," never "got unlucky." A deterministic step budget
  (§3.8) can bound how long the search is allowed to run, but doing so
  introduces a distinct third outcome rather than quietly weakening
  this guarantee — see §3.11.
- **One dimensional** — the algorithm's own data model is a flat,
  index-addressed `std::vector` of cells. It has no concept of a grid,
  rows, columns, or neighbors. All spatial/relational structure is
  supplied by the caller as constraints over cell indices. See §3.1
  for why this is what makes both a classic 1D WFC use case *and* a
  2D puzzle like Sudoku expressible on top of the same core, and
  `ISSUES.md` for the ambiguity this resolves.
- **Scales to large waves** — Sudoku (81 cells) is the showcase, not
  the ceiling. The library must not assume its caller only ever has a
  handful of cells; a procedural-generation caller with thousands of
  cells is an explicit target, which is why §3.9's algorithm is
  worklist-driven and trail-based rather than the simpler "re-scan
  everything, copy the wave per branch" approach a small-scale-only
  design would get away with.

The showcase application, `src/apps/sudoku` (`antwika::sudoku` /
binary `antwika_sudoku`), solves a Sudoku puzzle by expressing its
givens as an initial 81-cell wave and its rows/columns/3x3-box rules
as constraints over that flat array, then hands both to
`antwika::wfc::Solver` — no 2D-grid code inside the library at all.
Sudoku itself is small enough that it doesn't exercise the
weighted-entropy or step-budget features (§3.8, §3.9); it uses the
library's plain defaults, which is itself the point — the added
generality for larger callers must not tax the simple case.

## 2. Non-goals

- No built-in grid/adjacency geometry (2D, hex, or otherwise). The
  library only ever sees `std::vector<Domain>` plus a list of
  constraints over indices into it — geometry is entirely an
  application concern, expressed by which indices a constraint
  touches, the same way `apps/life`'s `Grid` maps `(x, y)` to `Entity`
  entirely outside `antwika::ecs`.
- No RNG/PRNG anywhere, matching the project-wide "Won't have." A
  weighted-entropy mode (§3.7) uses fixed, caller-supplied weight data,
  not randomness — it changes tie-breaking, not determinism.
- No general SAT/SMT solving. Two constraint kinds ship
  (`AllDifferentConstraint`, `AdjacencyConstraint`, §3.4-3.5) — enough
  for classic tile-adjacency WFC and for Sudoku's row/column/box
  rules. The `IConstraint` interface is the extension point for any
  future kind; inventing more than these two isn't needed today.
- No incremental/step-by-step *public* solving API for visualization.
  `solve()` is one-shot: given a wave and constraints, it returns a
  result. This is unrelated to the step budget in §3.8/§3.9, which
  exists purely to bound worst-case runtime deterministically and
  returns nothing about intermediate states — a future step-by-step
  observer hook remains a documented possible extension, not required
  here.
- No multithreaded search.
- No wall-clock/real-time timeout. §3.8's budget is measured in
  algorithm-defined steps (candidate values attempted), the same
  "discrete steps, not wall-clock time" principle
  `antwika::engine`'s fixed timestep already applies, per explicit
  instruction — see §3.8.
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

**What changed from the first draft, and why**: the original sketch
had `collapse()` copy the entire wave on every branch taken and pick
the next cell to collapse via an `O(n)` scan of every cell, and left
weighting and a runtime bound as "left entirely open." That was fine
at Sudoku's 81-cell scale but would degrade badly for a
procedural-generation caller with thousands of cells (an `O(n)` copy
or scan per decision point turns into `O(n^2)` or worse over a whole
solve). §3.7-3.9 below replace that with: an internal, incrementally
updated entropy index (no full rescan per pick), a single up-front
wave copy plus a trail-based undo log instead of one copy per branch,
an iterative explicit-stack search instead of recursion (so search
depth is never bounded by the C++ call stack), and an optional
per-value weight table for weighted entropy. None of this changes the
public shape of `IConstraint`/`AllDifferentConstraint`/
`AdjacencyConstraint`/`Domain` from the first draft, except that
`Domain` gains one more small operation (§3.2).

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
    void add(std::size_t value);       // restores a candidate; only
                                        // ever called by Trail (§3.9)
                                        // to undo a prior remove()
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
`add()` is the one addition versus the first draft: it is what lets
`Trail::rewindTo()` (§3.9) restore an exact prior domain bit by bit
without needing to re-derive it from scratch.

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

Unchanged from the first draft. One small interface, same shape as
`ISystem`/`IEventSink` elsewhere — a constraint is anything that can
look at (and narrow) the domains of the cells it cares about.
`prune()` doubles as the *only* consistency check the solver needs
(§3.9): collapsing a cell to a value and then re-running `prune()` on
every constraint touching it is what both propagates the consequence
and detects a violation, with no separate "is this assignment legal"
method required. Note that `prune()` itself stays a plain, trail-
unaware function — `Solver` is the only thing that knows about the
trail (§3.9), diffing each constraint's domains before/after the call
to record exactly what changed. Constraint authors never have to
think about undo logic.

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
the backtracking search in §3.9, not from how aggressively propagation
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
    LimitExceeded,   // step budget (§3.8) reached before either of
                     // the above could be determined
};

struct SolveResult
{
    SolveOutcome outcome;
    std::vector<std::size_t> assignment;  // valid iff outcome == Solved
};
```

`LimitExceeded` is new versus the first draft (§3.8). It is a
deliberately distinct value from `Unsatisfiable`: the search did not
prove there's no solution, it simply ran out of budget while still
searching. Conflating the two would silently break the completeness
guarantee (§1) by letting "gave up" masquerade as "proven impossible."

### 3.7 Weighted entropy

Classic WFC implementations often bias which cell collapses next (and
implicitly, generation "style") using per-symbol weights — e.g. a
procedural terrain generator might want `grass` picked far more often
than `water`, expressed as tile frequency. Sudoku has no such notion
(every digit is equally valid), but a procedural-generation caller
plausibly does, so this is opt-in, not required:

```cpp
Solver(std::vector<Domain> initialWave,
       std::vector<std::reference_wrapper<const IConstraint>>
           constraints,
       std::vector<double> valueWeights = {},   // empty == uniform
       SolverLimits limits = {});                // §3.8
```

`valueWeights[v]` is symbol `v`'s weight, shared across every cell in
the wave (the classic WFC model: a tile's frequency doesn't depend on
*where* it's being considered). An empty vector means "every value
has weight 1.0" — plain, unweighted MRV (minimum remaining values),
exactly the first draft's behavior.

Entropy for a cell, given its remaining candidate values `V` and their
weights:

```
W = sum(valueWeights[v] for v in V)
H = log(W) - sum(valueWeights[v] * log(valueWeights[v]) for v in V) / W
```

the standard weighted Shannon-entropy formula WFC implementations use.
When every weight is `1.0`, this reduces to `log(|V|)` — a strictly
increasing function of candidate count alone, so cell selection order
with uniform (or absent) weights is *identical* to the first draft's
plain-count MRV rule. Weighted entropy is therefore a strict
generalization: Sudoku passing nothing extra behaves exactly as
before, and a weighted caller gets biased selection without the
solver needing two separate code paths.

**Selection rule** (used by `EntropyIndex`, §3.9): among cells with
`count() > 1`, pick the one with the smallest `H`; ties (all-equal `H`,
which is exactly what happens whenever weights are uniform or absent)
broken by lowest cell index — always an exact integer comparison,
never a floating-point one, so the *tie-break* stays fully
deterministic regardless of any floating-point subtlety in computing
`H` itself. See §3.10 for the determinism claim this rests on.

### 3.8 Step budget (`SolverLimits`)

```cpp
struct SolverLimits
{
    std::optional<std::uint64_t> maxSteps;  // default: unlimited
};
```

Per explicit instruction: the solver must be boundable so it "can't
continue forever," but not via wall-clock time — the same "fixed,
discrete steps, not wall-clock time" principle
`REQUIREMENTS.md` already states for `antwika::engine`'s tick loop
applies here. One "step" is one candidate value attempted at a
decision point (§3.9's search loop increments a counter exactly once
per candidate tried, regardless of how much propagation work that
candidate triggers) — a deterministic, algorithm-defined unit: the
same wave/constraints/limit always exhausts the same budget at the
same point, run after run, machine after machine.

When the counter reaches `maxSteps`, `solve()` returns
`SolveOutcome::LimitExceeded` immediately (§3.6). Default
(`std::nullopt`) is unlimited, so Sudoku and the small 1D demo (§7)
are unaffected unless a caller opts in — this is a safety valve for
large/adversarial searches, not a behavior change for existing small
ones.

### 3.9 `Solver`: propagation, selection, and backtracking at scale

```cpp
class Solver
{
public:
    Solver(std::vector<Domain> initialWave,
           std::vector<std::reference_wrapper<const IConstraint>>
               constraints,
           std::vector<double> valueWeights = {},
           SolverLimits limits = {});

    [[nodiscard]] SolveResult solve() const;

private:
    std::vector<Domain> initialWave;
    std::vector<std::reference_wrapper<const IConstraint>> constraints;
    std::vector<double> valueWeights;
    SolverLimits limits;

    // Built once at construction: cellToConstraints[c] lists every
    // constraint whose cells() includes c. Lets propagation wake up
    // only the constraints actually touched by a change, instead of
    // re-scanning every constraint on every step.
    std::vector<std::vector<std::size_t>> cellToConstraints;
};
```

Constructor validates that every `Domain` in `initialWave` shares the
same alphabet size, and that every constraint's `cells()` indices are
in range, throwing `WfcError` otherwise (§3.12) — a cheap, purely
defensive check at the boundary, not a hot-path cost. It also builds
`cellToConstraints` once, up front.

`solve()` makes exactly **one** copy of `initialWave` (not one per
branch, unlike the first draft) into a local working `wave`, then
drives everything else through two small private helpers and an
explicit stack — no recursion, so search depth is never limited by
the C++ call stack, which matters once "large procedural generation"
(§1) means potentially many thousands of decision points deep:

- **`Trail`** (private, §4): an undo log of individual
  `(cellIndex, value)` removals. `Solver` snapshots a constraint's
  `cells()` domains before calling `prune()`, diffs after, and pushes
  one trail entry per value that disappeared — regardless of whether
  it came from `remove()` or `restrictTo()` clearing several bits at
  once. `trail.checkpoint()` returns the current length;
  `trail.rewindTo(checkpoint, wave, entropyIndex)` replays entries
  after that point in reverse, calling `Domain::add()` to restore each
  one and notifying `entropyIndex.update()` (below) so the index stays
  consistent. This is what replaces the first draft's per-branch wave
  copy: undoing a failed branch is proportional to how much that
  branch actually changed, not to the whole wave's size.
- **`EntropyIndex`** (private, §4): an incrementally maintained
  structure (e.g. an ordered-by-entropy index with a per-cell reverse
  lookup — the exact internal structure is an implementation detail,
  left open in §10) supporting `update(cell, domain)` (called whenever
  a cell's domain changes, whether shrinking during propagation or
  growing back during a rewind) and `pickNext()` (the next cell to
  collapse per §3.7's rule) in better than `O(n)` time each, so
  choosing the next cell to collapse never re-scans the whole wave.

Propagation, worklist-driven rather than "repeat full passes until
nothing changes" (the first draft's approach, which re-checks every
constraint on every pass regardless of whether it could possibly have
new information):

```
propagate(wave, trail, entropyIndex, startingWorklist):
    worklist = startingWorklist        // which constraints to (re)check
    while worklist not empty:
        constraint = worklist.popFront()
        before = snapshot(wave, constraint.cells())
        if not constraint.prune(wave): return false     // contradiction
        for each cell in constraint.cells() whose domain shrank:
            trail.record one entry per value removed
            entropyIndex.update(cell, wave[cell])
            for each other constraint touching that cell:
                if not already queued: worklist.pushBack(it)
    return true
```

The very first `propagate()` call in `solve()` seeds `worklist` with
every constraint once; every later call (after collapsing one cell)
seeds it with only the constraints touching that one cell — the cost
of re-propagating after a collapse scales with how connected that
cell is, never with the total wave size.

Search, an explicit loop with a choice-point stack instead of
recursion:

```
solve():
    wave = copy of initialWave                 // the only copy made
    trail = Trail{}
    entropyIndex = EntropyIndex(wave, valueWeights)
    steps = 0

    if not propagate(wave, trail, entropyIndex, allConstraints):
        return {Unsatisfiable, {}}

    stack = []   // ChoicePoint{cell, remaining candidates, checkpoint}

    loop:
        cell = entropyIndex.pickNext()
        if cell is null: return {Solved, extract(wave)}   // all singleton

        if stack empty or stack.top().cell != cell:
            push ChoicePoint{cell, ascending candidates of wave[cell],
                              trail.checkpoint()}

        top = stack.top()
        if top.remaining candidates is empty:
            pop stack
            if stack empty: return {Unsatisfiable, {}}
            trail.rewindTo(stack.top().checkpoint, wave, entropyIndex)
            continue

        if limits.maxSteps and steps >= *limits.maxSteps:
            return {LimitExceeded, {}}
        ++steps

        trail.rewindTo(top.checkpoint, wave, entropyIndex)  // undo the
                                          // previous candidate at this
                                          // cell, if any, before trying
                                          // the next one
        value = top.remaining candidates.popFront()
        restrict wave[cell] to {value}, recording the removals on
            trail and notifying entropyIndex
        propagate(wave, trail, entropyIndex, cellToConstraints[cell])
            // false: this candidate is a dead end; loop retries with
            // the next candidate at the same choice point. true: loop
            // continues -- pickNext() next iteration either surfaces a
            // new undetermined cell (a new choice point gets pushed)
            // or returns null (solved).
```

### 3.10 Determinism

Both which cell is picked at each step (§3.7's entropy rule, or plain
MRV with default weights) and the order candidate values are tried
(ascending) are fixed rules with no randomness, and no dependence on
container iteration order beyond `std::vector`/`Domain`'s own
documented index order — this holds regardless of the worklist/trail
mechanics in §3.9, which only change *how fast* the answer is reached,
never *which* answer. Running `solve()` twice on the same
wave/constraints/weights/limits (via one `Solver` or two independently
constructed ones) always returns the exact same `SolveResult`.

One caveat specific to weighted entropy (§3.7): `H` is computed with
`double` arithmetic, and while a fixed sequence of floating-point
operations on fixed inputs is itself deterministic on a given build,
this project targets three toolchains (GNU, LLVM, MinGW) and makes no
claim that `H`'s bit pattern is identical *across* them — only that
the *outcome* (which cell is picked, and hence the final assignment)
is reproducible for a given build, which is what "same input, same
output" determinism means everywhere else in this repo (e.g.
`EcsDeterminismTest`, `ReplayDeterminismTest`). Callers who want
cross-toolchain bit-identical weights are advised to stick to small
integer-valued weights (exactly representable in `double`), which
sidesteps the concern in practice; the plan does not chase further
than that, since nothing asked for here needs it.

### 3.11 Completeness

Every reachable branch at every decision point is tried, in order,
before the search reports `Unsatisfiable` — the search only gives up
on a cell once every one of its remaining candidate values has been
tried and led to a dead end further down. This holds exactly as
stated **only when `limits.maxSteps` is unset** (the default): with a
finite budget, the search may instead return `LimitExceeded`, which is
not a claim about solvability either way (§3.6, §3.8) — completeness
is preserved by keeping that outcome distinct, not by pretending a
truncated search still proves unsatisfiability. Worst-case time
remains exponential on adversarial inputs when no budget is set; §3.8
and `ISSUES.md` cover why a budget is offered instead of pretending
this away.

### 3.12 `WfcError`

One specific, catchable error type — mirrors `EcsError` and
`ReplayFormatError`'s existing "one exception type per library"
precedent — thrown only by `Solver`'s constructor, for mismatched
`Domain` alphabet sizes across the wave or an out-of-range constraint
cell index. Never thrown by `solve()` itself: an unsatisfiable puzzle,
and a budget-exceeded search, are both normal, non-exceptional
`SolveResult`s, not errors.

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
│   ├── SolverLimits.hpp       // SolverLimits (§3.8)
│   ├── WfcError.hpp
│   └── Solver.hpp
├── src/
│   ├── Domain.cpp
│   ├── AllDifferentConstraint.cpp
│   ├── CompatibilityTable.cpp
│   ├── AdjacencyConstraint.cpp
│   ├── Trail.hpp/.cpp             // private undo log, §3.9
│   ├── EntropyIndex.hpp/.cpp      // private incremental index, §3.9
│   └── Solver.cpp
└── tests/
    ├── CMakeLists.txt
    ├── DomainTest.cpp
    ├── AllDifferentConstraintTest.cpp
    ├── AdjacencyConstraintTest.cpp
    ├── TrailTest.cpp                 // record/checkpoint/rewindTo
    │                                  // restores exact prior domains
    ├── EntropyIndexTest.cpp          // update/pickNext in isolation,
    │                                  // uniform and weighted cases
    ├── SolverPropagationTest.cpp     // prune-only behavior, no search
    ├── SolverBacktrackingTest.cpp    // contradictions force backtrack
    ├── SolverCompletenessTest.cpp    // crafted solvable/unsatisfiable
    │                                  // CSPs, proves both outcomes
    ├── SolverDeterminismTest.cpp     // same input solved twice ->
    │                                  // bit-identical SolveResult
    ├── WeightedEntropyTest.cpp       // custom weights change
    │                                  // selection order; omitting
    │                                  // weights reproduces the
    │                                  // unweighted MRV order exactly
    ├── SolverStepLimitTest.cpp       // tiny maxSteps -> LimitExceeded,
    │                                  // not Solved or Unsatisfiable;
    │                                  // generous maxSteps still lets
    │                                  // a normal solve finish
    ├── SolverLargeScaleTest.cpp      // a few thousand cells, simple
    │                                  // constraints -> completes with
    │                                  // a valid assignment; a scale
    │                                  // regression check, not a timed
    │                                  // benchmark (§8)
    ├── OneDimensionalWfcTest.cpp     // classic tile-adjacency strip,
    │                                  // see §7 -- the literal 1D case
    └── mocks/
        └── include/antwika/wfc/mocks/MockConstraint.hpp
```

`CompatibilityTable` gets its own header/`.cpp` rather than living
inside `AdjacencyConstraint.hpp` since it is a small, independently
testable value type (a square boolean matrix), the same granularity
`antwika::ecs` uses for e.g. `Entity`/`EcsError` as separate headers.
`Trail` and `EntropyIndex` are kept as private headers under `src/`,
not `include/`, the same way `antwika::ecs` keeps `EntityManager.hpp`
and `IComponentPool.hpp` internal — implementation details `Solver`
composes, never part of the public surface, but still directly unit
tested via their (private) headers, mirroring `EntityManagerTest.cpp`.

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
`antwika::wfc`. `apps/sudoku` constructs `Solver` with only the first
two constructor arguments (§3.9) — no `valueWeights`, no
`SolverLimits` — demonstrating that the larger-scale features added
in §3.7/§3.8 stay fully opt-in and impose nothing on a caller that
doesn't need them.

### 5.3 CLI (`main.cpp`)

Same shape as `apps/game`/`apps/life`'s `main.cpp`: parse `argv` for an
optional `--puzzle <path>`; if absent, fall back to a small, built-in
demo puzzle constant (an easy, well-known example), the same
`--record`-less "baked-in demo, or load one" pattern `demoScript()`
uses in `apps/game`. Print the input board, run `Solver::solve()`,
then print either the solved grid (`SolveOutcome::Solved`) or a
"no solution" message with a non-zero exit code
(`SolveOutcome::Unsatisfiable`). `SolveOutcome::LimitExceeded` cannot
occur here (no limit is set), but the CLI still handles it explicitly
rather than falling through, so the switch stays exhaustive.

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

`.github/workflows/build.yml`'s "Verify executables" step (§9, step
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
during planning for the same reason. Concrete candidate additions for
once this actually lands are recorded in `ISSUES.md` rather than
guessed at here, per explicit instruction.

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

`SolverLargeScaleTest.cpp` (§4, §8) extends this same idea to a few
thousand cells, specifically to exercise §3.9's scalability design —
`OneDimensionalWfcTest` stays small and readable as the *demonstration*
of the 1D use case, while `SolverLargeScaleTest` is the *stress*
regression proving the design doesn't quietly assume small input.

## 8. Testing strategy

- GoogleTest + CTest, one behavior tested alongside the code that
  introduces it, per the project's existing rule.
- `DomainTest`: bit operations (including `add()`), singleton/empty
  construction, iteration order (ascending), equality.
- `AllDifferentConstraintTest` / `AdjacencyConstraintTest`: pruning
  behavior in isolation, including the "already contradictory input"
  case returning `false`.
- `TrailTest`: `record`/`checkpoint`/`rewindTo` restores the exact
  prior state of every affected `Domain`, including a case with
  multiple removals from the same cell between two checkpoints.
- `EntropyIndexTest`: `pickNext()` returns the correct cell under both
  uniform and custom weights; `update()` keeps the index consistent
  across a sequence of shrink-then-restore (rewind) operations.
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
- `WeightedEntropyTest`: a crafted wave where two cells have equal
  candidate *count* but different weight distributions collapses the
  lower-entropy one first; the same wave solved with no weights given
  reproduces the first draft's plain-count MRV order exactly.
- `SolverStepLimitTest`: a tiny `maxSteps` on a search that would
  otherwise need many candidates tried returns `LimitExceeded`, never
  `Unsatisfiable`; a generously large `maxSteps` still lets a normal
  small solve finish as `Solved`.
- `SolverLargeScaleTest`: a few-thousand-cell 1D wave with simple,
  mostly-satisfiable adjacency constraints completes with a valid
  assignment. Asserts correctness (every constraint holds in the
  result) and that it finishes at all within the test framework's
  default timeout, not a specific wall-clock target — a scale
  regression check, not a performance benchmark, to avoid flaky
  timing-based assertions.
- `OneDimensionalWfcTest`: §7's small tile strip, asserting every
  adjacent pair in the final assignment is compatible per the table.
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
4. `Trail`, `EntropyIndex` (private internals) (+ tests for each, in
   isolation from `Solver`).
5. `SolveResult`, `SolverLimits`, `WfcError`, `Solver` (worklist
   propagation, then entropy-driven iterative backtracking) (+
   propagation/backtracking/completeness/determinism/weighted-entropy/
   step-limit tests).
6. `SolverLargeScaleTest` and `OneDimensionalWfcTest` — the literal 1D
   demo and its scale regression counterpart (§7).
7. `antwika::sudoku` scaffold (`CMakeLists.txt`, wired into
   `src/apps/CMakeLists.txt`, linking only `antwika::wfc`).
8. `Board` + `BoardFormatError` (+ tests).
9. `Puzzle` (initial wave + 27 constraints) (+ tests).
10. `main.cpp` CLI, `.github/workflows/build.yml` expected-binaries
    and `gcovr --exclude` updates (§6), `README.md` project-structure
    update.
11. Integration tests: known easy/hard puzzles, an unsatisfiable
    puzzle, determinism.
12. Final review pass: coverage, workflow verification, blog post —
    see `PLAN_WFC_CHECKLIST.md`'s closing items.

## 10. Open questions

- Exact internal data structure for `EntropyIndex` (§3.9) is left to
  implementation — the plan only fixes its complexity goal (better
  than `O(n)` per `pickNext()`/`update()`) and its interface, not its
  internals (e.g. an ordered map keyed by entropy value plus a
  per-cell reverse lookup is one reasonable option, not a mandate).
- Whether `SolverLimits` should eventually gain a second bound (e.g. a
  cap on total propagation work, not just candidates attempted) is
  left open; `maxSteps` alone is judged sufficient for now since every
  unit of search work happens either as part of trying a candidate or
  as propagation directly triggered by one.
- Whether `REQUIREMENTS.md` actually gains the lines proposed in
  `ISSUES.md` once this lands, or different ones, is left for after
  implementation (§6).
