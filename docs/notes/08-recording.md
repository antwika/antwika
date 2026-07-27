# 08 — Recording

**Status:** not started

## Rationale/motivation

Before anything can be serialized to a replay, it has to be captured
somewhere in order. The codebase already has almost exactly this shape:
`EventRecorder` implements both `IEventSink` (receives events) and
`IEventHistory` (returns them back out as a `vector<Event>`). The tick-aware
replay system needs the same shape, one level up, over `TimedEvent`. See
[PLAN.md §3.5](../PLAN.md#35-recording-a-tick-aware-sibling-of-eventrecorder).

## How it's satisfied

`ITimedEventSink`/`ITimedEventHistory` (mirroring `IEventSink`/
`IEventHistory`) plus `ReplayRecorder` (mirroring `EventRecorder`), living in
the `event` lib next to `EventRecorder`. Registered as one of
`TickedEventDispatcher`'s timed sinks, so every tick-stamped event — built-in
or custom — lands in it, satisfying "nothing that happens during a run is
excluded."

## Issues encountered

_(filled in during implementation)_
