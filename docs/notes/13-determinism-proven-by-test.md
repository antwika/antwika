# 13 — Determinism is proven, not asserted by inspection

**Status:** not started

## Rationale/motivation

The core requirement of the whole effort: "The game/engine must be able to
load replays to deterministically produce the same exact state," and "at
least one test asserts deterministic replay saving and loading." Everything
in items 04–11 exists to make this test possible and meaningful — in
particular, [item 10](10-replay-same-code-path.md) (replay uses the exact
same code path as live) is what makes this test prove something structural
rather than something that happens to pass today.

## How it's satisfied

Two tests, at two levels:
- `replay` lib level: run A (live, scripted events) → record → serialize to
  an in-memory stream → deserialize → run B (fresh engine, sourced from the
  replay) → assert run A's and run B's folded state (a small test-local
  reducer) are equal, and their recorded histories are element-wise equal.
- `apps/game` level: the same shape, using the real `GameState`/
  `GameStateReducer` from [item 12](12-state-example-apps-game.md), which is
  the more meaningful, end-to-end version of the claim.

## Issues encountered

_(filled in during implementation)_
