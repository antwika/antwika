# 05 — Automatic tick-stamping of every dispatched event

**Status:** done. `TickedEventDispatcher` landed, decorating
`IEventDispatcher`: forwards every dispatched `Event` to the wrapped
dispatcher unchanged, then fans a `TimedEvent{currentTick, event}` out to its
registered `ITimedEventSink`s. `setTick(Tick)` lets the tick-loop
orchestrator (upcoming) advance the current tick between dispatches.

## Rationale/motivation

Requirement 4 in [PLAN.md §1](../PLAN.md#1-goal): "Everything that happens
during execution of engine must be replayable." If tick-stamping were
something each call site had to remember to do, it would be trivial to forget
for one event kind and silently produce a replay that's incomplete or
mis-ordered. Doing it centrally, once, in the dispatch path guarantees no
event can reach a sink without a tick attached to it.

## How it's satisfied

`TickedEventDispatcher` decorates the existing `IEventDispatcher` rather than
modifying it — see [item 06](06-extendable-events.md) and
[PLAN.md §3.4](../PLAN.md#34-tick-stamping-a-decorator-not-a-change-to-the-tested-eventdispatcher)
for why a decorator was chosen over editing the already-tested
`EventDispatcher`. It tracks the current tick and wraps every dispatched
`Event` into a `TimedEvent{tick, event}` before fanning out to `ITimedEventSink`s.

## Issues encountered

_(filled in during implementation)_
