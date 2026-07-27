# 18 — Definition of done

**Status:** not started

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

_(filled in at the end of implementation)_
