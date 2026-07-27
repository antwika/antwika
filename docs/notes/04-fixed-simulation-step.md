# 04 — Fixed simulation step (`Tick`)

**Status:** `Tick` type landed. Fixed-timestep `Engine::step()` is tracked
separately in this note once the engine rework lands (see PLAN.md §5 steps
1 and 7 — this note covers both).

## Rationale/motivation

Requested explicitly: "It is expected that the engine will operate on fixed
time." Determinism requires that "what happens" is a function of *discrete,
countable steps*, not of wall-clock time — two runs on different machines
(different CPU speed, different scheduling jitter) must reach the same state
given the same inputs, which is only possible if the thing state actually
advances on is a step counter, not `IClock::now()`. See
[PLAN.md §3.1](../PLAN.md#31-tick-lives-in-the-time-lib) for the type, and
[PLAN.md §3.3](../PLAN.md#33-fixed-timestep-requires-the-engine-to-own-a-tick-loop-and-gets-its-own-built-in-events)
for the loop that consumes it.

Decision made during planning discussion: `Tick` lives in the `time` lib
(`antwika::time::Tick`), not a new `simulation` lib — fewer moving parts, and
`time` is already the natural home for "how the engine measures progress."

## How it's satisfied

`using Tick = std::uint64_t;` added to `antwika::time`. `IEngine` gains
`step(Tick)`; a small orchestrator drives `step()` once per tick, in order,
for a fixed `Δt` per tick — never reading wall-clock time to decide how far
to advance.

## Issues encountered

_(filled in during implementation)_
