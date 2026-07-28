# Issues and open judgment calls

Found while researching and writing `PLAN_WFC.md` /
`PLAN_WFC_CHECKLIST.md`. Nothing here blocks the plan, but each is a
judgment call worth the user's explicit confirmation before
implementation starts, since a wrong guess here would be expensive to
unwind later.

## 1. "One dimensional" vs. a 2D showcase (Sudoku)

The request asks for a WFC library that is "one dimensional" and
*also* a Sudoku showcase, and Sudoku is visually a 9x9 grid. Read
literally, those two asks look contradictory.

**Resolution used in the plan**: "one dimensional" is applied to the
library's own data model and API — a flat, index-addressed
`std::vector<Domain>` with zero built-in notion of rows, columns, or
neighbors. All spatial structure (Sudoku's rows/columns/boxes, or a
classic tile strip's left/right adjacency) is expressed entirely by
the *caller* as constraints over flat indices. `PLAN_WFC.md` §3.1 and
§7 cover this, including a literal 1D demo (`OneDimensionalWfcTest`)
kept in the library's own test suite so "one dimensional" is
demonstrated directly, not only inferred from Sudoku's index math.

If "one dimensional" was actually meant more narrowly (e.g. the
library should refuse to be used for anything but a literal 1D
sequence, making the Sudoku showcase a different, non-WFC solver
instead), that changes the shape of this plan substantially — worth
confirming before implementation starts.

## 2. `apps/sudoku` intentionally skips the engine/replay stack

Both existing apps (`apps/game`, `apps/life`) are built around
`antwika::engine`'s fixed-timestep tick loop and `antwika::replay`'s
record/replay mechanism. `apps/sudoku` as planned does neither — it
links only `antwika::wfc` and runs as a one-shot "read a puzzle, solve
it, print the result" CLI. `PLAN_WFC.md` §5.5 explains why: a puzzle
solve isn't an ongoing simulation, so there's no tick to drive and
nothing meaningful to record or replay.

This is a deliberate scope decision, but it is a real deviation from
the shape of every existing app in the repo. If consistency with
`apps/game`/`apps/life`'s structure is more important than avoiding
manufactured ceremony, the plan would need a tick-loop wrapper around
the solve instead. **Not yet confirmed either way** — carried forward
unresolved from the first review pass.

## 3. Worst-case search performance — resolved: a deterministic step
budget, plus a scale-aware algorithm

**Original concern**: `Solver::solve()` was a complete backtracking
search with no bound at all, and its internals (copy-the-whole-wave
per branch, rescan-every-cell for entropy) were sized for an 81-cell
Sudoku board, not for "may be used for large procedural generation
tasks" — a pathological instance, or simply a large one, could run for
a very long time or degrade quadratically or worse.

**Resolved, per explicit direction**:

- A deterministic, step-counted budget (`SolverLimits::maxSteps`,
  `PLAN_WFC.md` §3.8) — *not* wall-clock time, matching the same
  "fixed discrete steps, not real time" principle
  `antwika::engine`'s tick loop already uses. Reaching the budget
  returns a new, distinct `SolveOutcome::LimitExceeded`, deliberately
  kept separate from `Unsatisfiable` so a truncated search can never
  be mistaken for a proof of unsatisfiability (`PLAN_WFC.md` §3.11).
- The "can't assume only small puzzles" direction changed more than
  just the budget: `PLAN_WFC.md` §3.9 replaced the original per-branch
  wave copy with a single up-front copy plus a `Trail`-based undo log,
  replaced the `O(n)` per-pick entropy rescan with an incrementally
  updated `EntropyIndex`, replaced "repeat full passes over every
  constraint" with worklist-driven propagation that only re-checks
  constraints actually touched by a change, and replaced recursive
  `collapse()` with an iterative loop over an explicit choice-point
  stack so search depth is never bounded by the C++ call stack. None
  of this changes `IConstraint`/`Domain`'s public shape (`Domain`
  gains one method, `add()`) — it's confined to `Solver`'s internals.
- Still true even after this: worst-case time is exponential on an
  adversarial instance when no `maxSteps` is set (that's inherent to
  exhaustive backtracking search, not fixable without giving up
  completeness) — the budget is the mitigation, not a claim that the
  search itself became polynomial.

## 3a. Weighted entropy — resolved: added as an opt-in generalization

**Original non-goal**: the first draft explicitly excluded weighted
entropy, reasoning that Sudoku doesn't need it.

**Resolved, per explicit direction**: added anyway, since it's "worth
adding" for procedural-generation callers even though Sudoku doesn't
need it. `PLAN_WFC.md` §3.7: an optional, empty-by-default
`valueWeights` constructor parameter; the entropy formula reduces
exactly to plain candidate-count MRV when weights are uniform/absent
(a strict generalization, not a second code path), so this is
additive and does not change Sudoku's behavior or complicate its
`Puzzle`/`main.cpp` code at all (§5.2 constructs `Solver` with only
the first two arguments). One caveat worth restating here: weighted
entropy compares `double` values, and while a fixed sequence of
floating-point operations on fixed inputs reproduces the same result
run after run *on a given build*, this project builds with three
different toolchains (GNU/LLVM/MinGW) and the plan makes no claim that
the entropy value's bit pattern matches bit-for-bit *across* them —
only that each toolchain's own output is reproducible
(`PLAN_WFC.md` §3.10). This mirrors this repo's existing determinism
claims (`EcsDeterminismTest`, `ReplayDeterminismTest`), which are also
per-build reproducibility claims, not cross-toolchain bit-identity
claims — but calling it out here since it's the first place in this
plan floating-point arithmetic enters a determinism-sensitive path.

## 4. `REQUIREMENTS.md` left unedited during planning — candidate
additions recorded here instead

Following the precedent set while planning `antwika::ecs` (its own
`docs/PLAN.md` §9 made the same call at the time), `REQUIREMENTS.md`
is not edited as part of this plan — it documents already-built,
current state, and is meant to be revisited only after the code
actually lands. Per explicit direction, candidate additions are
recorded here now instead of guessed at in `REQUIREMENTS.md` directly,
for the implementer to weigh once `antwika::wfc`/`apps/sudoku` are
real:

- **Must have**: `antwika::wfc::Solver` must be deterministic (no
  RNG) and, when no step budget is set, complete — it backtracks
  exhaustively rather than randomly retrying, so a returned
  `Unsatisfiable` is a proof, not a guess.
- **Must have**: a caller may bound `Solver::solve()`'s search via a
  deterministic, step-counted budget (`SolverLimits::maxSteps`), not
  wall-clock time; exceeding it must return a distinct
  `SolveOutcome::LimitExceeded` rather than misreporting
  `Unsatisfiable`.
- **Should have**: `antwika::wfc` should support optional
  weighted-entropy cell selection (per-value weights, defaulting to
  uniform) for procedural-generation callers, without requiring
  simpler callers (e.g. `apps/sudoku`) to specify any weights at all.
- **Could have**: `apps/sudoku` is a WFC showcase app that
  intentionally does not use `antwika::engine`/`antwika::replay` —
  worth a line clarifying that not every app is required to sit on
  the tick-loop/replay stack, once/if that's actually how it ships
  (see item 2 above, still unconfirmed).

Whether these exact lines (wording, section) are what should land, or
whether the implementer judges differently once the code exists, is
left entirely open — these are starting drafts, not commitments.

## 5. Plan file location and naming

The ECS library's equivalent planning docs lived at `docs/PLAN.md` and
`docs/CHECKLIST.md`, later deleted once the design was captured in
code and a blog post. This plan instead uses root-level
`PLAN_WFC.md`/`PLAN_WFC_CHECKLIST.md`, matching the file names given
in the request. If the user actually wants these under `docs/` for
consistency with the ECS precedent instead, that's a trivial rename —
called out here rather than assumed silently.
