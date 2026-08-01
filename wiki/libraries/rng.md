# antwika::rng

`src/libs/rng/` — seeded pseudo-random bits, and nothing else.

## What it is for

Producing a reproducible stream of 64-bit values from a seed.
It is standalone and dependency-free — it depends on nothing but the standard library, links no other `antwika` library and knows nothing about ticks, events or replay — but it is what several things a replay regenerates are built on.
That is [`wfc`](wfc.md)'s shape and it is chosen for the same reason: a caller wanting a deterministic generator should not have to link a poker engine to get one.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IRng.hpp` | `IRng` | One method, `next()`, returning 64 raw bits. |
| `SplitMix64Rng.hpp` | `SplitMix64Rng` | splitmix64: adds, shifts, xors and multiplies over a 64-bit counter. |

`FakeRng` lives under `tests/fakes/`, handing back a scripted sequence so a test can state the exact swap positions a shuffle will use.

## Depends on

Nothing.

## What determinism means here

The repository's central claim is that a recorded session replays to the same state.
Anything a replay regenerates rather than stores is bound by that claim, and a shuffle is the clearest case: [`apps/poker`](../apps/poker.md) records the deposits and the buy-ins and *nothing about the cards*, because the cards are a function of `RoomConfig::seed`.

So the guarantee is stronger than "the same seed gives the same stream in one process".
It is that the same seed gives the same stream **on every toolchain the project builds for, in every build configuration, and in every future revision of this library**.

## Non-obvious decisions

**The output sequence is part of the contract, not an implementation detail.**
`SplitMix64RngTest` pins the first three draws for seed 0 against the published splitmix64 output rather than against itself, so a rewrite that is merely self-consistent still fails.
Changing that expectation invalidates every checked-in replay in the repository at once, which is the size of alarm the change deserves.

**No `<random>`, no floating point, and no distributions of any kind.**
The standard engines are portable but the *distributions* are not: `std::uniform_int_distribution` is free to consume a different number of draws and produce a different value on libstdc++ and libc++, and `std::shuffle` is specified only to permute.
A shuffle that differed between the GNU and LLVM legs of CI would be a replay divergence with the symptom nowhere near the cause.
`rand()` is out for the same reason, and additionally because it is ambient global state.

`SplitMix64Rng` is a fixed sequence of additions, shifts, xors and multiplies over an exact-width `std::uint64_t`, which behaves identically on every target.
Its state is 64 bits and it takes any seed including zero, so there is no "bad seed" a caller has to know about.

**One method, returning raw bits.**
There is deliberately no `nextInRange()`, no `nextFloat()` and no distribution.
Adding one would put the interesting arithmetic *inside* the library and out of sight, which is where a portability difference hides best.
Leaving it out means every bounded draw is written at its call site, in the open, next to the code whose determinism depends on it — `rng.next() % (index + 1)` in [`holdem`](holdem.md)'s Fisher-Yates, `rng.next() % config.height` in [`apps/tower_defence`](../apps/tower_defence.md)'s level generator.
Those are visibly modulo-biased, and that is stated rather than hidden: a level layout and a card shuffle need reproducibility, not uniformity to the last bit.
A caller that genuinely needs an unbiased bounded draw writes rejection sampling at its own call site, where the extra draws it consumes are part of that caller's stream and not a silent property of everyone's.

`next()` is `noexcept` and `[[nodiscard]]`.
Discarding a draw is almost always a bug — it silently advances the stream — so a caller that means to skip one assigns it.

**Injected or local, never ambient.**
There is no global generator, no thread-local one, and nothing under `src/` seeded from a clock or a `std::random_device`.
An `IRng` is a constructor argument, as `holdem::Deck` takes one, or a local built from a seed something already persists: `poker::RoomConfig::seed`, `game::SaveGame::seed`, `td::LevelConfig::seed`.
That is what makes the interface worth having at all with a single production implementation — `FakeRng` lets a test state the swap positions a shuffle will use rather than reverse-engineering them from a seed.

[`wfc`](wfc.md) takes no seed at all and is deterministic on its input, so it is not a caller: in [`apps/game`](../apps/game.md)'s world map the seed reaches the solver only through which anchor cells are pinned, which is the one place randomness enters.

## What is not in here

There is no positional hash — `hash(seed, x, y) -> value` — for randomness that is a function of a coordinate rather than of call order.
Nothing in the repository needs one: every caller draws in a fixed order from a fixed seed, which the sequential generator already answers.
It is the obvious next addition, and it belongs here rather than in the app that first wants it.

There is no jump-ahead or stream-splitting either.
Where two things need to be independent, they get separate seeds derived at the call site — [`apps/tower_defence`](../apps/tower_defence.md) spaces its per-attempt seeds by the golden-ratio constant splitmix64 steps its own state by — which is one visible line rather than an interface.
