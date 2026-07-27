# 06 — Events are extendable

**Status:** done. `Event::payload` landed, and `apps/game`'s
`game.score_increment` (see [item 12](12-state-example-apps-game.md)) is the
proof: an application-defined event, unknown to the engine core, flowing
through the identical `TimedEvent`/`ITimedEventSink`/replay pipeline as the
engine's own built-in `engine.tick`, with zero changes required anywhere in
`engine`, `event`, or `replay` to support it.

## Rationale/motivation

Requested explicitly: application code (e.g. `apps/game`, or any future game
built on this engine) must be able to define and use its own event kinds,
while still benefiting from the engine's own built-in events — and both must
flow through the same recording/replay pipeline uniformly.

A polymorphic `Event` hierarchy (subclasses per event kind) was considered
and rejected: it would need a name→decode factory/registry to reconstruct
concrete subclasses during deserialization, forces `unique_ptr` + cloning to
keep value semantics, and none of that machinery is actually required to get
the same extensibility. See
[PLAN.md §3.2](../PLAN.md#32-extending-event-without-breaking-it-and-without-a-class-hierarchy)
for the full comparison.

## How it's satisfied

`Event` gains an opaque `payload: std::string` field alongside the existing
`name`. The engine's own mechanisms (dispatch, queue, record, serialize,
replay) only ever need to know "an event has a name and some bytes" — they
never need to change to support a new event kind, whether it's a built-in
the engine ships (see [item 07](07-builtin-common-events.md)) or one an
application invents. Encoding/decoding the payload's meaning is entirely the
application's business.

## Issues encountered

_(filled in during implementation)_
