# 18 — Definition of done

**Status:** done.

## Rationale/motivation

A closing checkpoint so "done" is an objective, checkable state rather than
a feeling — every item above satisfied, the full test suite green, the
unused-test-double guard green, and a commit history that actually shows the
incremental process rather than one large opaque diff.

## How it's satisfied

Before this item is checked off:
- Every item 01–17 above is checked off, with its note updated to
  `Status: done` and any issues encountered recorded.
- `ctest --test-dir build --output-on-failure` passes in full.
- `python3 scripts/check_unused_test_doubles.py` passes.
- `git log` shows a sequence of small, Conventional-Commits-formatted local
  commits corresponding to the items above.

## Issues encountered

None at this checkpoint. Verified directly:
- All items 01–17 checked off in `docs/CHECKLIST.md`, each with a
  `Status: done` note in `docs/notes/`.
- `ctest --test-dir build --output-on-failure`: 71/71 tests passing.
- `python3 scripts/check_unused_test_doubles.py`: OK, 11 test doubles
  checked, none orphaned.
- `git log --oneline`: 13 local commits, one-line Conventional Commits
  messages, none pushed (`main` is 13 commits ahead of `origin/main`).

The most significant thing this implementation surfaced wasn't a checklist
gap but a design correction found *by* writing the required determinism
test (see [item 13](13-determinism-proven-by-test.md)): a run's full
recorded history and a replay's required input are different things, and
conflating them silently breaks determinism rather than proving it. That
distinction is now documented in `PLAN.md §3.5` and carried through
`apps/game`'s actual `--record`/`--replay` wiring, not just the tests.
