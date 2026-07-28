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
the solve instead.

## 3. Worst-case search performance is unbounded

`Solver::solve()` (`PLAN_WFC.md` §3.7) is a complete backtracking
search with no step limit or timeout. Constraint propagation plus the
minimum-remaining-values heuristic make this fast in practice for
Sudoku-sized puzzles and small WFC demos, but a pathological or
adversarially constructed unsatisfiable instance could still take a
long time to exhaustively prove unsatisfiable. No timeout/step-limit
mechanism is planned initially (`PLAN_WFC.md` §10); flagging in case
the library is ever expected to run against untrusted or much larger
input than a 9x9 Sudoku board or a short tile strip.

## 4. `REQUIREMENTS.md` left unedited during planning

Following the precedent set while planning `antwika::ecs` (its own
`docs/PLAN.md` §9 made the same call at the time), `REQUIREMENTS.md`
is not edited as part of this plan — it documents already-built,
current state, and is meant to be revisited only after the code
actually lands. Noting this explicitly so it isn't mistaken for an
oversight later.

## 5. Plan file location and naming

The ECS library's equivalent planning docs lived at `docs/PLAN.md` and
`docs/CHECKLIST.md`, later deleted once the design was captured in
code and a blog post. This plan instead uses root-level
`PLAN_WFC.md`/`PLAN_WFC_CHECKLIST.md`, matching the file names given
in the request. If the user actually wants these under `docs/` for
consistency with the ECS precedent instead, that's a trivial rename —
called out here rather than assumed silently.
