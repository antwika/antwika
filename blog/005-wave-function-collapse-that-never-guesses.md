# Wave Function Collapse that never guesses

*Post 5*

Every prior library in this repo has had a shape imposed by something
outside it: `antwika::ecs` by the double-buffered `World` it has to
support, `antwika::replay` by the events it has to serialize
bit-for-bit. `antwika::wfc` is the first one built from a set of
properties decided up front — deterministic, complete, one
dimensional, and scalable — with no existing caller dictating any of
them. `apps/sudoku` is the proof those four properties actually hold
on something recognizable, not just on synthetic test fixtures.

## What "no RNG" costs you, and what it buys back

Classic Wave Function Collapse implementations pick the next cell and
the next candidate value with a random number generator, and they're
allowed to fail on contradiction — hit a dead end, throw the whole
attempt away, reseed, try again. That's a reasonable design when
"failure" just means "regenerate the level," but it means the
algorithm can never tell you a puzzle has no solution, only that this
particular random walk didn't find one.

This library goes the other way on purpose. Cell selection is
lowest-entropy-first with ties broken by cell index; candidate values
at a chosen cell are tried in ascending order. Both are fixed rules,
so `solve()` on the same wave and constraints returns the exact same
`SolveResult` every time — `SolverDeterminismTest` and
`SudokuDeterminismTest` just call it twice and compare. The trade is
that giving up on a branch can never mean "bad luck, try a different
seed" — it has to mean every remaining candidate at every open choice
point was actually tried and failed. `SolverCompletenessTest` exists
specifically to prove that: a hand-crafted unsatisfiable CSP has to
come back `Unsatisfiable`, not silently hang or misreport.

Worst case, exhaustive search on an adversarial input is exponential
— determinism and completeness don't remove that, they just make
"it's still searching" and "it's actually done" distinguishable.
That's what `SolverLimits::maxSteps` and `SolveOutcome::LimitExceeded`
are for: a third outcome, distinct from both `Solved` and
`Unsatisfiable`, so a caller with a step budget can bound runtime
without the result quietly lying about whether a solution exists.
`SolverStepLimitTest` checks a tiny budget returns `LimitExceeded`
rather than either of the other two, and that a generous one still
lets a normal small solve finish.

## One copy, not one copy per branch

The design that first got sketched out for this library copied the
entire wave at every branch point — fine for Sudoku's 81 cells, a
real problem for a caller with a few thousand. What shipped instead
copies `initialWave` exactly once, at the top of `solve()`, and
undoes everything else through a trail:

```cpp
void Trail::rewindTo(
    std::size_t checkpoint,
    std::vector<Domain> &wave,
    EntropyIndex &entropyIndex)
{
    while (entries.size() > checkpoint)
    {
        const auto [cell, value] = entries.back();
        entries.pop_back();
        wave[cell].add(value);
        entropyIndex.update(cell, wave[cell]);
    }
}
```

Every value a constraint's `prune()` removes gets one entry recorded
against the constraint pass that removed it, whether it came from a
single `remove()` or a `restrictTo()` that cleared several bits at
once — `Solver` diffs each constraint's domains before and after the
call rather than asking constraints to manage undo themselves, so
`IConstraint` implementations stay simple functions with no notion of
a trail at all. Abandoning a branch costs exactly as much as that
branch actually changed, not the size of the whole wave.

Picking the next cell to collapse has the same "don't rescan
everything" shape. `EntropyIndex` keeps a `std::set<std::pair<double,
std::size_t>>` ordered by (entropy, cell index), with a per-cell
reverse lookup so `update()` can remove a cell's stale entry before
reinserting its new one:

```cpp
void EntropyIndex::update(std::size_t cell, const Domain &domain)
{
    if (cellKey[cell].has_value())
    {
        keysByEntropy.erase(*cellKey[cell]);
        cellKey[cell].reset();
    }
    if (domain.count() <= 1)
    {
        return;
    }
    const std::pair<double, std::size_t> key{
        computeEntropy(domain), cell};
    keysByEntropy.insert(key);
    cellKey[cell] = key;
}
```

`pickNext()` is then just `keysByEntropy.begin()`. Singleton and empty
domains are never inserted at all, so the set only ever holds cells
still worth choosing between — the same structure serves both a
shrinking domain during propagation and a domain growing back during
`rewindTo`, which is what `EntropyIndexTest` checks by driving it
through a shrink-then-restore sequence rather than only ever shrinking
it. `SolverLargeScaleTest` is the regression that actually exercises
the payoff: a few thousand cells with simple adjacency constraints,
asserted only to finish within the test framework's default timeout
and to produce a valid assignment — a scale check, not a timed
benchmark.

## Where the plan's pseudocode was wrong

`PLAN_WFC.md` §3.9 sketched the main search loop as: call
`entropyIndex.pickNext()` at the top of every iteration, and treat
"nothing left to pick" as "solved." That's correct after a
*successful* `propagate()`, but the pseudocode also runs it right
after a *failed* one, on the way to trying the next candidate at the
same choice point. `EntropyIndex` only ever tracks cells with
`count() > 1` — a contradiction can leave some *other* cell's domain
empty without that cell ever having been in the index to begin with,
so "pickNext returns nothing" doesn't reliably mean "solved" at that
point; it could just as easily mean "the wave is broken and nobody's
been forced to notice yet."

The implementation restructured the loop so a failed `propagate()`
falls straight through to the next candidate at the same choice point,
and `pickNext()` is only ever consulted right after a propagate that
actually succeeded (including the very first one, before any choice
point exists). Every other guarantee from the plan — the single wave
copy, worklist-driven propagation, ascending candidate order, the
iterative explicit-stack search, the step-counted budget — is exactly
as designed; this was a correctness fix to the loop shape around them,
not a change to what any of them do.

## Flat indices, twice

The library's own rule is that it never sees a grid — only
`std::vector<Domain>` and constraints over indices into it.
`apps/sudoku` is the test of whether that rule survives contact with
something that's visually a 9x9 grid to every human who's ever solved
one. It does, entirely inside `Puzzle::buildConstraints()`: row `r` is
indices `r*9 .. r*9+8`; column `c` is `{c, c+9, ..., c+72}`; box `b`'s
nine cells come from `(3*(b/3) + dr)*9 + (3*(b%3) + dc)` for `dr, dc`
in `0..2`. All 27 of those become `AllDifferentConstraint`s, and
`Solver` never has any idea it's solving something 9x9 — it just sees
27 sets of 9 indices each.

`OneDimensionalWfcTest` demonstrates the same core doing the thing WFC
usually means when people say the name: a short strip of cells, a
`grass`/`sand`/`water` alphabet, and `AdjacencyConstraint` wired
between every consecutive pair through a `CompatibilityTable` that
says which symbols may sit next to each other. Sudoku proves the core
generalizes past literal adjacency to an arbitrary constraint graph;
the tile strip proves it wasn't secretly specialized to Sudoku's shape
to get there. Neither test knows the other exists, and that's the
point — one core, two unrelated problems, zero shared code beyond
`antwika::wfc` itself.

`apps/sudoku` also tests the other end of the plan's non-goals
directly: it links only `antwika::wfc` — no `antwika::engine`,
`antwika::replay`, `antwika::time`, `antwika::log`. Solving a puzzle
is one batch computation with no tick to drive and nothing to record
or replay, so it doesn't reach for machinery built for `apps/game` and
`apps/life`'s ongoing simulated worlds just because those two already
existed. Weighted entropy and the step budget stay opt-in for the same
reason from the other direction: Sudoku's `Solver` is constructed with
only a wave and constraints, and the demo puzzle solves purely through
propagation before backtracking has anything to do at all — the
generality `antwika::wfc` carries for a large procedural-generation
caller costs Sudoku nothing it isn't using.

---

`PLAN_WFC.md` and `PLAN_WFC_CHECKLIST.md` are deleted alongside this
post, per the disposition decided before any of this code existed —
guidance for getting here, not a record worth keeping once it's done.
