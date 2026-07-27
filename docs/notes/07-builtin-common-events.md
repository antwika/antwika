# 07 — Built-in, common event(s) from the engine

**Status:** not started

## Rationale/motivation

Extensibility ([item 06](06-extendable-events.md)) is only half of "while
still benefiting from common events that come with the engine" — the engine
needs to actually ship at least one such event, or the claim is untested.
The natural candidate is a per-tick "a tick happened" event: it's something
literally every application built on this engine will want to react to
(timers, decay, periodic logic) without having to manually dispatch its own
tick signal every step.

## How it's satisfied

`Engine::step(tick)` dispatches a named constant event (`events::kTick`,
`"engine.tick"`) through the dispatcher before processing that tick's queued
events. The name is a named constant (not a magic string duplicated between
engine code and consumers) in a small `antwika/engine/Events.hpp` header.
Application code (see [item 12](12-state-example-apps-game.md)) reacts to it
through the exact same `ITimedEventSink` mechanism it uses for its own custom
events — no special-casing between "built-in" and "custom."

## Issues encountered

_(filled in during implementation)_
