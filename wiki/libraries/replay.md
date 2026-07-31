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
| `SchemaVersion.hpp` | `kReplayDocumentVersion`, `kTickEventSchemaVersion`, `kSchemaVersionKey` | The versions, named here rather than written at a call site. |
| `IMigration.hpp`, `MigrationChain.hpp` | `IMigration`, `MigrationChain` | Single-step N-to-N+1 migrations, applied until a document is current. |
| `ReplayMigrations.hpp` | `standardReplayMigrations()` | The replay document's own chain, constructed and injected. |
| `ReplayFormatError.hpp` | `ReplayFormatError` | Bad replay input. |
| `SchemaVersionError.hpp` | `SchemaVersionError` | Narrower: a document this build cannot bring to the current version. |
| `EngineLoopError.hpp` | `EngineLoopError` | Loop misuse. |
| `ReplayCli.hpp`, `CommandLine.hpp`, `FlagSpec.hpp`, `CommandLineError.hpp` | `ReplayCliOptions`, `CommandLine`, `FlagSpec` | The `--record <path>` / `--replay <path>` flags every app shares. |
| `TickPacer.hpp` | `TickPacer` | An `ecs::ISystem` that sleeps through an injected `time::ISleeper` so a windowed run does not spin. |
| `TickEventRecorder.hpp` | `TickEventRecorder` | Records what was dispatched, minus each app's self-generated events. |
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

**Every persisted schema states its version.**
The replay document carries it in its own `"version"` member; the tick-event schema carries it in its `$id`, because an event repeats thousands of times per document and its revision is fixed by the document holding it.

Reading is `parse → read version → migrate → validate → decode`, and validating *after* migrating is what lets exactly one schema exist rather than one per revision.
A document with no version member is version 1, which is what every file written before this says.

`MigrationChain` applies single-step migrations so the number of them stays linear in bumps rather than quadratic, and it is generic over an `nlohmann::json` and a version key — so a save file uses the same mechanism with its own list and its own current version.
Chains are constructed and injected, never registered globally.

See [`docs/schema-versioning.md`](../../docs/schema-versioning.md) for what counts as a breaking change.

**Drawing more often than the simulation ticks is a decorator, not a change to the loop.**
`EngineLoop::run()` is the one path live and replay runs share, and putting a render cadence in it would change its signature for every app to give one of them a feature.
[`app`](app.md)'s `FramePacedSource` wraps an `IReplaySource` instead: it draws the extra frames in the gap before a tick's events are read, then returns what the inner source returned, unchanged.
It is a pure observer in exactly `PointerHintSource`'s sense.

**`ReplaySource::eventsFor()` scans linearly.**
An index has deliberately not been built, because replays are not yet long enough for it to matter.
