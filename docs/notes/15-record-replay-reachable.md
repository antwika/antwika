# 15 — Record/replay reachable from the actual application

**Status:** done. `bootstrap()` now takes an `IReplaySource &inputSource`
and `Tick totalTicks`, and drives an `EngineLoop` for the real run (both
live and replay modes call the identical function). `main.cpp` gains
`--record <path>` (runs a hardcoded demo script, writes it to a file) and
`--replay <path>` (loads a file, replays it) flags — smoke-tested manually:
a live `--record` run and a subsequent `--replay` of the saved file produce
byte-identical log output and the identical final `GameState`. The
requested "test asserts deterministic replay saving and loading" now exists
at the actual application level: `ReplayIntegrationTest.cpp` runs
`bootstrap()` live, serializes the input script, deserializes it,
runs `bootstrap()` again from the loaded replay, and asserts the two
resulting `GameState`s are equal — through the same entry point `main.cpp`
uses, not a test-only shortcut.

## Issues encountered

Deciding *what* to serialize as "the replay" was the crux of
[item 13](13-determinism-proven-by-test.md)'s finding: it's the input
script (the `vector<TimedEvent>` used to build the live run's
`ReplaySource`), not `ReplayRecorder`'s full observed history. In this
demo's scope (no real interactive input source yet), "recording" a replay
turned out not to need any recording step in `bootstrap()` at all — the
caller (`main.cpp`, the test) already holds the input script it's about to
run, and can serialize that directly. If/when true interactive live input
is added later, capturing *that* into a saveable script becomes a real,
separate piece of work (a recorder attached specifically to
externally-originated dispatches, distinct from `Engine::step()`'s
internally-generated ones) — flagged here for whoever picks that up.

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
