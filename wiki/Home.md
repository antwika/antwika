# Antwika wiki

Antwika is a C++23 game and engine project built with CMake and Conan, tested with GoogleTest through CTest, and developed inside VS Code Dev Containers (GNU, LLVM, MinGW) for a reproducible toolchain.

It is a monorepo of small, single-purpose libraries under `src/libs/`, composed by applications under `src/apps/`.
The one idea everything else follows from is *deterministic replay*: the simulation advances in fixed ticks, every external input arrives as an `Event`, and a live run and a replayed run go through the same loop, differing only in where each tick's events come from.

These pages are plain markdown with relative links, so they read on GitHub and in an editor with no tooling at all.
They are also shaped so the directory could later be pushed to a GitHub wiki repository verbatim, but nothing here sets that up.

## Map

- [Getting Started](Getting-Started.md) — dev containers, the Conan/CMake/CTest cycle, backend selection, and how to run each app.
- [Architecture](Architecture.md) — the layering, the tick loop, the determinism rule, and the library dependency graph.
- [Contributing](Contributing.md) — the worktree rule, the checker scripts, the coverage gate, and commit conventions.
- [Glossary](Glossary.md) — tick, event, replay source, sink, system, backend, snapshot, and the rest of the vocabulary.

## Libraries

Each page covers what the library is for, its key headers and types, what it depends on, and the decisions in it that are not obvious from the code.

| Library | One line |
| --- | --- |
| [engine](libraries/engine.md) | Steps one fixed tick at a time and knows nothing about any app. |
| [event](libraries/event.md) | The one uniform event mechanism: `Event`, `TickEvent`, dispatchers and sinks. |
| [replay](libraries/replay.md) | The shared live/replay loop, the JSON replay format, and the CLI flags. |
| [ecs](libraries/ecs.md) | Double-buffered entity/component `World` with staged mutations. |
| [scheduler](libraries/scheduler.md) | Deterministic, priority-ordered, budget-bounded job dispatch. |
| [wfc](libraries/wfc.md) | Wave Function Collapse over a flat cell array, with no grid concept inside. |
| [holdem](libraries/holdem.md) | No-limit Texas hold'em rules, side pots, and a hand evaluated into one number. |
| [gfx](libraries/gfx.md) | Backend-agnostic windows and a write-only renderer. |
| [ui](libraries/ui.md) | Immediate-mode layout that produces a picture as a value. |
| [input](libraries/input.md) | Backend-agnostic keyboard and pointer edges, delivered as replay input. |
| [time](libraries/time.md) | The `Tick` type, `IClock` and `ISleeper`. |
| [log](libraries/log.md) | Composable logging with no global state. |
| [app](libraries/app.md) | The wiring every app's `main.cpp` would otherwise repeat. |

## Applications

| App | One line |
| --- | --- |
| [game](apps/game.md) | An isometric grid you build on with the mouse, with the camera as simulation state. |
| [life](apps/life.md) | Conway's Game of Life held in an ECS world, toggled by dragging. |
| [task_worker](apps/task_worker.md) | A worker pool pulling tasks off the deterministic scheduler. |
| [poker](apps/poker.md) | A no-limit hold'em cash game with bankrolls, hand histories and a drawn table. |
| [sudoku](apps/sudoku.md) | A Sudoku solved as constraint propagation, with no tick loop at all. |
| [gfx_demo](apps/gfx_demo.md) | The graphics and UI showcase: bars, a texture, and clickable buttons. |

## Where else to look

- [`README.md`](../README.md) — the project's front page.
- [`REQUIREMENTS.md`](../REQUIREMENTS.md) — every design constraint phrased as a MoSCoW requirement; this is the closest thing to a specification.
- [`docs/STYLE_GUIDE.md`](../docs/STYLE_GUIDE.md) — the authoritative coding style.
- [`blog/`](../blog/) — after-the-fact write-ups explaining *why* a piece was built the way it was.
