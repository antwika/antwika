# 15 — Record/replay reachable from the actual application

**Status:** not started

## Rationale/motivation

A replay system that only exists inside unit tests isn't actually usable.
The original request was for the game/engine (i.e. the real `apps/game`
application) to be able to load replays — so record/replay needs a real
entry point in `bootstrap()`/`main.cpp`, not just library-level plumbing.

## How it's satisfied

`bootstrap()` gains record and replay entry points; `main.cpp` gains minimal
`--record <file>` / `--replay <file>` argv handling (no new dependency).
Exercised by the `apps/game`-level integration test from
[item 13](13-determinism-proven-by-test.md), which uses the same
`bootstrap()` entry points against an in-memory stream (a real file isn't
needed to prove correctness, and keeps the test fast/hermetic).

## Issues encountered

_(filled in during implementation)_
