# antwika::replay

`src/libs/replay/` — the loop, the file format, and the CLI.

## What it is for

Running the simulation, and recording or reproducing a run.
This is where the project's central claim lives: live and replay runs go through the same code path, so a replay reproduces state by construction.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `EngineLoop.hpp` | `EngineLoop` | Per tick: ask the source for events, dispatch them, step the engine, check `StopSignal`. |
| `IReplaySource.hpp` | `IReplaySource` | `eventsFor(tick)` — the single seam between live and replayed runs. |
| `ReplaySource.hpp` | `ReplaySource` | Serves events out of a loaded `ReplayDocument`. |
| `ReplayDocument.hpp` | `ReplayDocument` | The in-memory form of a replay file. |
| `ReplayReader.hpp` / `ReplayWriter.hpp` | `ReplayReader`, `ReplayWriter` | JSON in and out; `ReplayWriter::Layout` is `Compact` or `Pretty`. |
| `EventJson.hpp`, `PayloadJson.hpp`, `ReplayJson.hpp` | — | The JSON encoding, validated with `json-schema-validator`. |
| `ReplayFormatError.hpp` | `ReplayFormatError` | Bad replay input. |
| `EngineLoopError.hpp` | `EngineLoopError` | Loop misuse. |
| `ReplayCli.hpp`, `CommandLine.hpp`, `FlagSpec.hpp`, `CommandLineError.hpp` | `ReplayCliOptions`, `CommandLine`, `FlagSpec` | The `--record <path>` / `--replay <path>` flags every app shares. |
| `TickPacer.hpp` | `TickPacer` | An `ecs::ISystem` that sleeps through an injected `time::ISleeper` so a windowed run does not spin. |
| `WindowInputSource.hpp` | `WindowInputSource` | An `IReplaySource` that turns a window close request into `engine.stop`. |
| `CanvasCheck.hpp` | `CanvasCheck` | Guards a replay recorded against one canvas size from being replayed against another. |

## Depends on

[`ecs`](ecs.md), [`engine`](engine.md), [`event`](event.md), [`gfx`](gfx.md), [`log`](log.md), [`time`](time.md).
`ecs` is there for `TickPacer` and `gfx` for `WindowInputSource` and the recorded canvas size — both narrow, deliberate edges.

## Non-obvious decisions

**Only externally-supplied events are persisted.**
Anything the engine regenerates on its own — `engine.tick` above all — is never written.
Each app names its self-generated events so the recorder can drop them, and no `input.*` name may ever appear in such a list.

**Closing a window is input, not control flow.**
`WindowInputSource` decorates another source and appends `engine.stop` once the window has gone, so a close lands in a `--record` file like any other input and replays at the same tick.
Nothing is allowed to short-circuit the loop.

**Layout is chosen by who reads the file.**
`saveReplayFile()` writes `Compact`, because a `--record` run is read by `ReplayReader` rather than by a person.
The demo replays checked in under `src/apps/*/replays/` stay `Pretty`, so they stay diffable and hand-editable.

**`ReplaySource::eventsFor()` scans linearly.**
An index has deliberately not been built, because replays are not yet long enough for it to matter.
