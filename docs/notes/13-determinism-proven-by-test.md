# 13 — Determinism is proven, not asserted by inspection

**Status:** done, at the `replay` lib level (`ReplayDeterminismTest.cpp`).
The `apps/game`-level version, using the real `GameState`/`GameStateReducer`,
lands with [item 12](12-state-example-apps-game.md)/[item 15](15-record-replay-reachable.md).

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

Writing this test caught a real duplication bug, exactly the kind "tests
alongside, verify behavior" ([item 01](01-tests-alongside.md)) exists to
catch. The first version built the "replay to reload" by serializing
`ReplayRecorder`'s **full observed history** of the live run — which, by
[item 08](08-recording.md)'s design, correctly includes every dispatched
event, including the engine's own built-in `engine.tick` (item 07). Feeding
that full history back in as `EngineLoop`'s replay *source* meant
`engine.tick` got dispatched **twice** per tick on replay: once because it
was present in the loaded source, and once again because `Engine::step()`
*always* dispatches a fresh one, live or replayed, by design. The replayed
run's state hash and event count came out different from the live run — the
test failed exactly as it should have.

The fix is conceptual, not a patch: **a run's full observed history and its
replay *input* are different things.** The built-in tick is a pure,
deterministic function of the tick number — `Engine::step()` regenerates it
identically every time, so it must never *also* be supplied as input, or it
double-fires. What a replay actually needs to store is the *input script* —
in this codebase's current scope (no live/interactive input source yet),
that's exactly the `vector<TimedEvent>` used to build the `IReplaySource`
that drove the original run in the first place — not the derived,
strictly-larger history `ReplayRecorder` observes. The test now serializes
`scriptedLiveEvents` (the input) rather than `liveRecording.getEvents()`
(the full history), and separately asserts the two runs' full histories are
equal as the actual determinism check — which now holds, because both runs
are driven by the identical input through the identical code path.

This has one open implication for [item 15](15-record-replay-reachable.md):
when record/replay is wired into `apps/game`, "saving a replay" must save
the input events dispatched from *outside* `Engine::step()`, not
`ReplayRecorder`'s full history. `ReplayRecorder` remains valuable in its
own right as a complete audit trail (item 08 is unchanged), it's just not
what gets handed to `BinaryReplayWriter` to build a reloadable file. See the
implementation-note callout added to
[PLAN.md §3.5](../PLAN.md#35-recording-a-tick-aware-sibling-of-eventrecorder).
