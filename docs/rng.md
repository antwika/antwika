# Randomness

`antwika::rng` is the project's one source of pseudo-random bits.
It is two headers -- `IRng` and `SplitMix64Rng` -- and it depends on nothing but the standard library, in the spirit of [`antwika::wfc`](../src/libs/wfc): a caller wanting a deterministic generator must not have to link a poker engine to get one.
This note records what "deterministic" is promising here, and why the interface is one method returning raw bits.

## What determinism means in this library

The repository's central claim is that a recorded session replays to the same state.
Anything a replay regenerates rather than stores is therefore bound by that claim, and a shuffle is the clearest case: `apps/poker` records the deposits and the buy-ins and *nothing about the cards*, because the cards are a function of `RoomConfig::seed`.

So the guarantee is stronger than "the same seed gives the same stream in one process".
It is that the same seed gives the same stream **on every toolchain the project builds for, in every build configuration, and in every future revision of this library**.
Two consequences follow, and both are deliberate.

**The sequence is part of the contract, not an implementation detail.**
`SplitMix64RngTest` pins the first three draws for seed 0 against the published splitmix64 output rather than against itself, so a rewrite that happened to be self-consistent still fails.
Changing that expectation invalidates every checked-in replay in the repository at once, which is exactly the size of alarm the change deserves.

**No floating point, and no `<random>`.**
The standard engines are portable but the *distributions* are not: `std::uniform_int_distribution` is free to consume a different number of draws and produce a different value on libstdc++ and libc++, and `std::shuffle` is specified only to permute.
A shuffle that differs between the GNU and LLVM legs of CI would be a replay divergence with the symptom nowhere near the cause.
`rand()` is out for the same reason, and additionally because it is ambient global state.

`SplitMix64Rng` is a fixed sequence of additions, shifts, xors and multiplies over an exact-width `std::uint64_t`, which behaves identically on every target.
Its state is 64 bits and it is seeded with any value including zero, so there is no "bad seed" a caller has to know about.

## One method, returning raw bits

`IRng::next()` returns 64 bits and that is the entire interface.
There is deliberately no `nextInRange()`, no `nextFloat()` and no distribution of any kind.

Adding one would put the interesting arithmetic *inside* the library and out of sight, which is where a portability difference hides best.
Leaving it out means every bounded draw is written at its call site, in the open, next to the code whose determinism depends on it -- `rng.next() % height` in `apps/tower_defence`'s level generator, `rng.next() % (index + 1)` in `holdem::Deck`'s Fisher-Yates.
Those are visibly modulo-biased, and that is fine and stated rather than hidden: a level layout and a card shuffle need reproducibility, not uniformity to the last bit.
A caller that genuinely needs an unbiased bounded draw writes rejection sampling at its own call site, where the extra draws it consumes are part of that caller's stream and not a silent property of everyone's.

`next()` is `noexcept` and `[[nodiscard]]`.
Discarding a draw is almost always a bug -- it silently advances the stream -- so a caller that means to skip one assigns it.

## Injected, never ambient

An `IRng` is a constructor argument, as `holdem::Deck` takes one, or a local, as `apps/game`'s world generator and `apps/tower_defence`'s level generator each build one from a seed.
There is no global generator, no thread-local one, and nothing seeded from a clock or a `std::random_device` anywhere under `src/`.
A seed always comes from something a replay or a save already holds: `poker::RoomConfig::seed`, `game::SaveGame::seed`, `td::LevelConfig::seed`.

That is what makes the interface worth having at all with a single production implementation: `fakes::FakeRng` hands back a scripted sequence, so a test can state the exact swap positions a shuffle will use rather than reverse-engineering them from a seed.

## What is not in here

There is no positional hash -- `hash(seed, x, y) -> value` -- for randomness that is a function of a coordinate rather than of call order.
Nothing in the repository needs one yet: every caller today draws in a fixed order from a fixed seed, which the sequential generator already answers.
It is the obvious next addition, and it belongs here rather than in the app that first wants it.

There is no jump-ahead or stream-splitting either.
Where two things need to be independent, they get separate seeds derived at the call site -- `apps/tower_defence` spaces its per-attempt seeds by the golden-ratio constant splitmix64 steps its own state by -- which is one visible line rather than an interface.
