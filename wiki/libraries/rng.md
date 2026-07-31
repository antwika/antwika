# antwika::rng

`src/libs/rng/` — seeded pseudo-random bits, and nothing else.

## What it is for

Producing a reproducible stream of 64-bit values from a seed.
It is standalone and dependency-free — it links no other `antwika` library and knows nothing about ticks, events or replay — but it is what several things a replay regenerates are built on.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IRng.hpp` | `IRng` | One method, `next()`, returning 64 raw bits. |
| `SplitMix64Rng.hpp` | `SplitMix64Rng` | splitmix64: adds, shifts, xors and multiplies over a 64-bit counter. |

`FakeRng` lives under `tests/fakes/`, handing back a scripted sequence so a test can state the exact swap positions a shuffle will use.

## Depends on

Nothing.

## Non-obvious decisions

**The output sequence is part of the contract.**
`SplitMix64RngTest` pins the first three draws for seed 0 against the published splitmix64 output rather than against itself, so a rewrite that is merely self-consistent still fails.
Changing that expectation invalidates every checked-in replay in the repository at once, which is the size of alarm the change deserves.

**No `<random>`, and no distributions of any kind.**
The standard engines are portable but the distributions are not: `std::uniform_int_distribution` may consume a different number of draws and produce a different value on libstdc++ and libc++, and `std::shuffle` is specified only to permute.
So `IRng` returns raw bits and every bounded draw is written at its call site — `rng.next() % (index + 1)` in [`holdem`](holdem.md)'s Fisher-Yates, `rng.next() % config.height` in [`apps/tower_defence`](../apps/tower_defence.md)'s level generator — in the open, next to the code whose determinism depends on it.
The modulo bias that comes with that is stated rather than hidden; a shuffle and a level layout need reproducibility, not uniformity to the last bit.

**Injected or local, never ambient.**
There is no global generator, no thread-local one, and nothing under `src/` seeded from a clock or a `std::random_device`.
A seed always comes from something a replay or a save already holds.

See [`docs/rng.md`](../../docs/rng.md).
