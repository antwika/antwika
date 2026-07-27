# 02 — Commit per completed chunk, Conventional Commits

**Status:** in progress (ongoing constraint, re-affirmed at [18](18-definition-of-done.md))

## Rationale/motivation

Requested explicitly. Small, self-contained commits make the history a
readable log of *how* the feature was built (useful for review, bisection,
and reverting a single misstep without losing everything after it), rather
than one opaque diff. [Conventional Commits](https://www.conventionalcommits.org/)
matches this repo's existing history style (`fix: ...`, `docs: ...`,
`chore(release): ...`).

## How it's satisfied

A commit is made once a checklist item (or a natural, independently-green
sub-chunk of one) builds and its tests pass. Types used in this effort:
`feat:` for new engine/library capability, `test:` where a commit is
primarily a test addition with no behavior change, `docs:` for plan/checklist
notes and README updates, `refactor:` for structural changes with no
behavior change.

## Issues encountered

None yet.
