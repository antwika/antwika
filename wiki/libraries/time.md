# antwika::time

`src/libs/time/` — the tick, the clock and the sleeper.

## What it is for

The vocabulary for time in the project, kept to the smallest thing that works.
It carries no tick-loop or replay logic of its own; it just defines what a tick *is* and lets a clock and a sleep be injected.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Tick.hpp` | `Tick` | `using Tick = std::uint64_t;` — the simulation's unit of time. |
| `IClock.hpp` | `IClock` | Reading wall-clock time, behind an interface. |
| `SystemClock.hpp` | `SystemClock` | The real implementation. |
| `ISleeper.hpp` | `ISleeper` | Waiting, behind an interface. |
| `SystemSleeper.hpp` | `SystemSleeper` | The real implementation. |

`FakeClock` and `FakeSleeper` live under `tests/fakes/`.

## Depends on

Nothing.

## Non-obvious decisions

**A tick is a count, not a duration.**
Nothing in the simulation ever sees a wall-clock delta, so nothing in it can depend on how fast the machine ran.
Wall-clock time exists only where a human is watching: [`log`](log.md) timestamps through `IClock`, and `replay::TickPacer` paces a windowed run through `ISleeper`.

**`ISleeper` exists so tests still run at full speed.**
A windowed app paces its ticks through an injected sleeper rather than calling a sleep function, so the same code under test with a `FakeSleeper` finishes instantly.
That is also what keeps the pacing outside the simulation rather than inside it.
