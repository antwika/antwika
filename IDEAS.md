# Application ideas

A running list of applications that would showcase the Antwika libraries, with what each one would actually *prove*.
Nothing here is committed work; this is a menu, not a plan.

## What makes a good showcase app here

The libraries in `src/libs/` are not general-purpose infrastructure looking for a user.
They encode a small number of opinions, and a showcase app is worth building when it puts one of those opinions under load.

- **It must be deterministic.** Same inputs, same ticks, same state — every time, on every backend, on every platform.
  An idea that needs wall-clock time, a hash-ordered container, an unseeded RNG or a floating-point reduction over an unspecified order is not a showcase, it is a bug report waiting to happen.
- **It must be replayable, and the replay must hold only external input.** If the app can regenerate something from the tick stream, the recording must not contain it.
  `apps/game` records the click and regenerates the tile; that is the shape every new app should take.
- **It must have nowhere to hide state.** State lives in an `ecs::World`, an `ITickEventSink` fold, or a plainly-owned struct the app admits to owning.
  A cache that a renderer keeps between frames, or a static local anywhere, disqualifies an idea until it is designed out.
- **Rendering must stay write-only.** A scene reads a snapshot and draws; it never answers a question the simulation asked.
- **It should be testable without a device.** The best ideas here are ones where the interesting part is a pure function over values, so a test asserts with `EXPECT_EQ` and no mock, the way `ui::DrawList` and `holdem::HandValue` do.
- **It should stress something that is currently unproven.** `wfc::AdjacencyConstraint` and `wfc::CompatibilityTable` have no application at all today; `scheduler` has exactly one; `input::ActionMap` and `input::Binding` have none.
  Those gaps are where the value is.

Difficulty below is rough: **small** is a weekend and one new module, **medium** is a new library or a substantial app, **large** is a new library plus an app plus a design document.

## Cellular and grid simulations (`ecs`, `event`, `replay`)

### Falling sand

A sandbox of sand, water, stone and fire on an integer grid, painted with the mouse the way `apps/life` toggles cells.
Each material is a rule about where a cell may move next, applied in a fixed scan order so that the result is a function of the order and nothing else.
It exercises `ecs`'s double-buffered `World` at a much higher cell count than Life, and `input`'s `IdleMotionSource`/`CoalescingPointerSource` trade-off becomes visible: a brush that skips cells because a tick coalesced two movements is a bug you can *see*.
Proves the ECS commit discipline scales past a toy, and that a drag is faithfully replayable at speed.
**Difficulty: small.**

### Ant colony with pheromone trails

Ants are ECS entities; the pheromone field is a flat integer grid that decays by a fixed integer amount per tick.
Foraging emerges from two rules and no floating point anywhere — pheromone strength is a `std::uint16_t`, and "follow the strongest neighbour" breaks ties by a fixed compass order rather than by chance.
Proves `replay` determinism for an emergent system, which is the hardest kind: a one-tick divergence is visible within seconds because the colony picks a different trail.
**Difficulty: medium.**

### Boids in fixed point

Flocking with separation, alignment and cohesion, computed entirely in Q16.16 fixed point so that the simulation is bit-identical across GNU, LLVM and MinGW.
The interesting deliverable is the fixed-point vector type itself, which several other ideas here would reuse.
Proves that "deterministic across toolchains" is a property this repo can actually claim, rather than one it has never been forced to test.
**Difficulty: medium.**

### Traffic lights

Cars advance along the road network `apps/game` already builds, obeying signals that cycle on a fixed tick schedule.
It reuses `game`'s grid, camera and atlas, so the new code is the vehicle system and the signal fold.
Proves the isometric grid and the camera-as-simulation-state decision hold up when something other than a walker moves on them.
**Difficulty: small**, given `apps/game`.

### Elevator bank

Several cars serve a building of floors against a queue of requests, under a dispatch policy that is a pure function of the pending requests and car positions.
The showcase is that swapping the policy swaps the whole behaviour with no other change, and a recorded set of requests replays identically under any policy.
Proves the "policy is a pure function of a view" pattern `poker::PolicyAgent` established, outside poker.
**Difficulty: small.**

## Constraint solving (`wfc`)

### Tile map generator

The application `wfc` was named for and has never had: generate a dungeon, a city block or an island using `AdjacencyConstraint` and `CompatibilityTable` over the tiles in `src/apps/game/assets/atlas.png`.
Seeded, so a map is a number you can share, and re-solvable tile by tile so the collapse can be watched one tick at a time.
Proves the half of `wfc` that no application currently touches, and would almost certainly find the first real bug in it.
**Highest value on this list — difficulty: medium.**

### Nonogram solver

Row and column run-length clues over a binary grid.
The clue is not an all-different constraint and not an adjacency constraint, so it needs a third `IConstraint` implementation — which is exactly the point.
Proves `IConstraint` is a genuine extension seam rather than a shape fitted to sudoku after the fact.
**Difficulty: small.**

### Kakuro and killer sudoku

Sum constraints over cell groups, layered on top of the all-different constraints `apps/sudoku` already expresses.
A `SumConstraint` that prunes a domain by arithmetic bounds is a natural second citizen beside `AllDifferentConstraint`.
Proves constraints compose over the same flat cell array without the library growing a grid concept.
**Difficulty: small.**

### Exam timetabler

Assign exams to (room, slot) pairs subject to "no student sits two exams at once" and "no room hosts two exams at once".
This is the same solver applied to something that is obviously not a puzzle, which is the argument that `wfc` is a constraint library rather than a puzzle library.
Proves the flat, index-addressed cell array is genuinely geometry-free.
**Difficulty: medium.**

### Crossword filler

Fill a grid from a word list, where each slot is a cell whose domain is the words that fit its length and each crossing is a constraint on a shared letter.
The domain is large, so this is the first real test of `SolverLimits` and of how gracefully a contradiction is reported.
Proves `wfc` survives a domain in the thousands rather than the nines of sudoku.
**Difficulty: medium.**

## Job scheduling (`scheduler`)

### Build graph simulator

A make-like dependency DAG where each target is an `IJob` with a duration, run against a bounded number of workers.
Load a real `compile_commands.json`-shaped graph and watch the critical path fall out.
Proves the `scheduler` DAG and per-tick budget against a graph that was not authored to make the scheduler look good — the current `apps/task_worker` submissions are.
**Difficulty: small**, given `apps/task_worker`.

### Production chain

Machines are ECS entities that consume inputs and produce outputs on recipes; a recipe becomes a scheduled job when its inputs are available.
Throughput and bottlenecks are then a property of the priority order, which is deterministic and inspectable.
Proves `scheduler` and `ecs` composing under a workload where the job graph is *generated* each tick rather than submitted by a script.
**Difficulty: medium.**

## Card and rules engines (`holdem`)

### Equity calculator

Given two hole-card ranges and a board, enumerate or seeded-sample the runouts and report win/tie/lose percentages.
It calls `holdem::evaluate()` millions of times and nothing else, so it is both a showcase and a benchmark.
Proves `HandValue`'s total ordering exhaustively — the strongest correctness argument available for that library is "we compared every 7-card hand against a reference and the ordering agreed".
**Difficulty: small.**

### Tournament manager

Blind levels that rise on a tick schedule, tables that break and re-seat as players bust, and a payout structure.
Table balancing is a deterministic rule over seat counts, so a whole tournament replays from its seed and its buy-ins.
Proves `holdem::TableRunner` composes into something larger than one table, and generalises `poker::BankrollLedger` past a cash game.
**Difficulty: medium.**

### Blackjack or hearts

A second card game built on `holdem::Card`, `IDeck`, `Deck` and `rng::IRng` without touching `Table` or the evaluator.
The value is negative space: it shows which parts of `holdem` are card primitives and which are poker rules, and would likely justify splitting a `antwika::cards` library out.
Proves the primitives are not secretly poker-shaped.
**Difficulty: small.**

## Tools for the repo itself (`replay`, `ui`, `gfx`)

### Replay inspector

Load any `.replay` JSON and show it as a scrubbable timeline: events per tick, filterable by name, with the payload of the selected event.
It reads through `replay::ReplayReader` and draws through `antwika::ui`, and it needs no simulation of its own.
Proves `ui`'s layout and interaction under a real, data-driven, scrolling list, and gives every other app on this list a debugger.
**Highest value on this list — difficulty: small.**

### Determinism gate

A command-line tool that runs a replay, hashes the app's state after every tick, and compares the sequence to a golden file checked in beside the replay.
Wire it into CI across all three toolchains and both graphics backends.
Proves the repo's central claim mechanically instead of by argument, and turns "replay is deterministic" from a design intent into a test that fails.
**Highest value on this list — difficulty: small**, once one app exposes a state hash.

### Atlas editor

Open `atlas.png`, show the slot grid `TileAtlas.hpp` addresses it with, and let a slot be picked and previewed under the isometric projection.
Now that the art is hand-drawn rather than generated, it is the only thing that would show an artist their tile in the projection it is blitted through, which is where a nearly-right diamond stops being nearly right.
Proves `gfx::PngReader`, texture sub-rect blitting and `ui` together in a tool rather than a demo.
**Difficulty: small.**

## Interaction and presentation (`input`, `ui`, `gfx`)

### Input conformance viewer

A window that draws every `InputEvent` as it arrives: the key names, the modifier state, the pointer position, the scroll notches, and what `capabilities()` reported.
Run it under SDL and under raylib side by side and the differences between a queue backend and a polled one become visible.
Proves the backend conformance suite's claims to a human, and would be the first user of `input::ActionMap` and `input::Binding`, which no application currently touches.
**Difficulty: small.**

### Text editor

A modal, keyboard-driven editor over an in-memory buffer, with every keystroke arriving as an `input.*` event and therefore recordable and replayable.
An editing session becomes a replay file, which is a genuinely useful artefact and a brutal test of input fidelity.
Proves `ui` text layout at a scale the demos do not reach, and forces the keyboard half of the input backends to be finished.
**Difficulty: large.**

### Chart widgets

Axes, line and bar series, and a legend, laid out arithmetically the way the rest of `antwika::ui` is, over a data series the app owns.
Every other idea here that produces numbers over time would then have somewhere to put them.
Proves the layout arena handles a widget whose content size is data-driven rather than text-driven.
**Difficulty: medium.**

## Games (`engine`, `event`, `replay`)

### Tetris or snake

The smallest possible twitch game on a fixed tick.
Every input is an edge, gravity is a tick count, and a whole game is a replay file small enough to paste into a bug report.
Proves that a fixed-tick loop is a viable basis for a reflex game, and would surface any latency the input decorators cost — which is the honest test of the `IdleMotionSource` trade-off `CLAUDE.md` describes.
**Difficulty: small.**

### Chess with a clock

Legal move generation as a pure function, state as an `ITickEventSink` fold, and a hand history written the way `poker::TablePrinter` writes one — but in PGN, which the rest of the world already reads.
The clock is tick-derived, so it replays exactly.
Proves the "export a format other tools consume" pattern generalises beyond hand histories.
**Difficulty: medium.**

### Turn-based tactics

A small squad-tactics game on a seeded, generated map, where a whole run is one replay file and a seed.
It would pull in `wfc` for the map, `scheduler` for turn order, `ecs` for units, `ui` for the interface and `input` for everything — the first app that uses nearly the whole repo at once.
Proves the libraries compose into a real game rather than into demos, which is the standing open question.
**Difficulty: large.**

### Lockstep netplay demo

Two processes running the same `EngineLoop` over the same event stream, exchanging only inputs, diverging never.
The transport is a new seam; the simulation is unchanged, which is the whole demonstration.
Proves the replay architecture is also a netcode architecture, which is the strongest possible argument for it.
**Difficulty: large.**

## New libraries this list would motivate

### `antwika::audio`

A programmable music player: patterns as pure functions of musical time, and envelopes modulating arbitrary parameters.
The PCM half of the original idea has shipped as `antwika::sound`, which decodes, mixes and plays through a build-time backend seam, so what is left here is the musical layer above it.
See [`docs/audio-player-plan.md`](docs/audio-player-plan.md) for the full design and for which phases landed.

The projection argument this was going to prove has been settled a different way, and it is worth saying so rather than leaving the claim standing.
Nothing runs on another thread: an `antwika::sound` device is pumped by the caller, so the hardest case the write-only rule was going to face has not arrived, and it arrives only if a backend that cannot be pumped ever does.
**Difficulty: large.**

### `antwika::pathfind`

A* and flow fields over an abstract graph, with the grid supplied by the caller the way `wfc` takes constraints rather than a geometry.
Deterministic tie-breaking is the entire design problem, and `apps/game`'s walkers are the immediate customer.
Proves that "the library holds no geometry" is a repeatable pattern rather than a one-off in `wfc`.
**Difficulty: medium.**

### `antwika::rng`

The lift has landed: `IRng` and `SplitMix64Rng` are `src/libs/rng/` now, and the three hand-copied splitmix64 implementations under `apps/poker`, `apps/game` and `apps/tower_defence` are one library between them.
See [`docs/rng.md`](docs/rng.md).

What is left is the positional hash (`hash(seed, x, y) -> value`), for anything that needs randomness as a function of position rather than of call order.
Nothing needs one yet — every caller today draws in a fixed order from a fixed seed — so it is waiting for its first customer rather than being built ahead of one.
**Difficulty: small.**

### `antwika::fixed`

Q16.16 fixed-point scalars and vectors with saturating arithmetic and no undefined behaviour.
Boids, physics, audio envelopes and any future 3D work all want it, and all want it to be the *same* one.
Proves cross-toolchain bit-identity is achievable, which the repo currently avoids needing by using integers everywhere.
**Difficulty: small.**

## Highest value next

- **Replay inspector.** It is small, it needs no new library, and it makes every future replay bug an hour's work instead of a day's.
- **Determinism gate.** The repo's central claim is currently defended by design argument alone; this turns it into a CI failure, and it is the cheapest insurance on the list.
- **Tile map generator.** Half of `wfc` — `AdjacencyConstraint` and `CompatibilityTable` — has never been run by an application, and unexercised code in a library that advertises determinism is the largest unknown in the tree.
- **Equity calculator.** It would establish `holdem::evaluate()`'s ordering exhaustively against a reference, which no amount of unit testing at the current scale can do.
- **Input conformance viewer.** `ActionMap` and `Binding` have no user, the raylib keyboard is still missing, and both of those are cheap to fix once something visibly depends on them.
- **Falling sand.** It is the fastest way to find out whether the ECS commit discipline and the input decorators hold up at a cell count and an event rate that Life never reaches.
