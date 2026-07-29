# JSON wins, TimedEvent becomes TickEvent, and three main()s stop repeating themselves

*Post 9*

The [previous post](008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md) was a review pass: bugs and rough edges found by reading nine libraries end to end against a new style guide.
This one is about a piece of that codebase that had been carrying an old decision for a while — the binary replay format from [post 1](001-building-a-deterministic-replay-system.md) — and what it took to finally retire it.

## A second format, on purpose

Dropping the binary codec outright would have meant designing its replacement and deleting the fallback in the same change, with no point where both existed side by side to compare.
So the first step added `EventJson`/`ReplayJson` and `JsonReplayReader`/`JsonReplayWriter` as a second, parallel format, alongside `BinaryEventCodec` and `BinaryReplayReader`/`Writer` rather than instead of them.
A replay document became `{"magic": "antwika-replay", "version": 1, "events": [{"tick": 1, "event": {"name": "...", "payload": "..."}}, ...]}`, validated on read against a schema (`EventSchema.hpp`) built from `nlohmann::json-schema-validator` rather than hand-rolled field checks.

The apps' event payloads moved with it.
`game.score_increment`'s payload had been a bare string like `"5"`; `life.toggle_cell`'s was `"x,y"`; `task.submit`'s was `"id,priority,durationTicks,label[,dependsOnId]"`.
All three became small JSON objects — `{"amount":5}`, `{"x":2,"y":3}`, `{"id":1,"priority":"High","durationTicks":3,"label":"..."}` — each validated against its own schema before the reducer or sink ever looked at a field.
That's strictly more ceremony than a comma-split string for the same information, but it's ceremony the JSON schema validator now enforces instead of each app re-deriving its own parsing and bounds-checking, which is exactly the kind of boundary [post 8](008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md) had just finished hardening by hand in three different places.

## TimedEvent becomes TickEvent

With both codecs live, one naming mismatch got harder to ignore: `TimedEvent`, `ITimedEventSink`, `ITimedEventHistory` all said "timed," while everything they sit next to — `Tick`, `TickedEventDispatcher`, `setTick()`, `engine.tick` — already said "tick."
`ITimedEventSink` read as a foreign concept bolted onto `IEventSink` instead of what it actually is: the tick-stamped counterpart to it.
The rename swept the whole family — `TimedEvent` → `TickEvent`, `ITimedEventSink` → `ITickEventSink`, `ITimedEventHistory` → `ITickEventHistory` — and picked up one more name along the way: `antwika::event::ReplayRecorder`, a class that records tick-stamped events for later replay, was named after `antwika::replay`'s concept rather than its own; it became `TickEventRecorder`, mirroring `EventRecorder` the way the pair was always meant to read.
Pure rename, no behavioral change — all 351 tests passed before and after, which is the whole point of doing a rename as its own commit instead of folding it into whatever functional change happened to be touching those files that week.

## The binary format goes, and three main()s stop repeating themselves

Once JSON had been living alongside binary for a full commit, dropping `BinaryEventCodec`, `BinaryPrimitives`, `BinaryReplayReader`/`Writer`, and `IEventCodec` was the easy part — no callers left to migrate, just deletion.
`JsonReplayReader`/`Writer` lost their now-redundant qualifier and became plain `ReplayReader`/`Writer`, since there was no longer a second format for the name to distinguish them from.

The more interesting cleanup was what fell out of having only one format left: each app's `main.cpp` — `game`, `life`, `task_worker` — had grown to about 160 lines by repeating the same shape.
Parse `--record`/`--replay` off `argv`.
Build a hand-authored `demoScript()` vector of events for the no-`--replay` case.
Filter the engine's own self-generated events (`engine.tick`, plus whatever unconditional startup line that app printed) out of what gets written back on `--record`.
The three copies differed in almost nothing but which event names to filter and which literal ticks and payloads made up that app's demo run.

That shared shape is now `antwika::replay::ReplayCli`: `parseReplayCliOptions()` for the flags, `loadReplayFile()` and `saveReplayFile()` for the read/write side, the latter taking a `std::span` of extra self-generated event names so each app still gets to say what's uniquely its own.
The `demoScript()` vectors are gone entirely, replaced by a checked-in `replays/demo.json` per app, loaded through the exact same `ReplayReader` a user-supplied `--replay` file goes through:

```json
{
  "magic": "antwika-replay",
  "version": 1,
  "events": [
    { "tick": 1, "event": { "name": "game.score_increment", "payload": "{\"amount\":5}" } },
    { "tick": 3, "event": { "name": "game.score_increment", "payload": "{\"amount\":2}" } },
    { "tick": 4, "event": { "name": "engine.stop", "payload": "" } }
  ]
}
```

That collapses what used to be two branches in every `main()` — a `--replay <path>` branch and a hardcoded-demo branch, each constructing a `ReplaySource` its own way — into one path with a single fallback value for which file to load.
Each `main.cpp` is under 80 lines now, down from around 160, and the demo scenario a developer can read is the same JSON a user's own recording would produce, not a parallel C++ representation of it that had to be kept in sync by hand.

One more piece of duplication came out at the same time, smaller but same shape: `BoardSink`, `GameStateReducer`, and `TaskSubmissionSink` each parsed their event's payload as JSON and validated it against a schema before touching a field, and each did it with its own copy of "try to parse, catch and rethrow as my own exception type; try to validate, catch and rethrow again."
`antwika::replay::parseAndValidatePayload<ErrorT>()` is that sequence factored out once, parameterized over the caller's exception type so `BoardSink` still throws `BoardSinkError` and `TaskSubmissionSink` still throws `TaskSubmissionError`, just through one shared implementation instead of three hand-copied ones.

Coverage stayed at 100% line/function/branch throughout — checked against the same `conan-coverage` + `gcovr` invocation CI runs — which mattered more here than usual, since deleting ~1,300 lines of binary codec and hand-rolled demo scripts is exactly the kind of change that can quietly take test coverage down with it if a deleted `.cpp`'s tests aren't fully accounted for by what replaced them.

## What's left

A trailing comment in `task_worker`'s `BootstrapTest.cpp` still pointed at "`main.cpp`'s `demoScript()`" for the scenario it was mirroring — a `demoScript()` that no longer exists anywhere in the file it named.
That's a one-line fix, but it's the same failure mode [post 8](008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md) was written about: a comment that was accurate when written and silently stopped being true once the code around it moved on.
It now points at `replays/demo.json`, which is the file actually worth reading to understand the scenario.
