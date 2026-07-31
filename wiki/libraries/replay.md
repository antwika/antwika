# antwika::replay

`src/libs/replay/` — the file format, the migrations, and the CLI flags.

## What it is for

Recording a run and reading one back.
Running the loop is [`simulation`](simulation.md)'s, and this library depends on it: `ReplaySource` is a `simulation::ITickSource`, and a live run needs nothing that is here.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `ReplaySource.hpp` | `ReplaySource` | The `simulation::ITickSource` that serves events out of a loaded `ReplayDocument`. |
| `ReplayDocument.hpp` | `ReplayDocument` | The in-memory form of a replay file. |
| `ReplayReader.hpp` / `ReplayWriter.hpp` | `ReplayReader`, `ReplayWriter` | JSON in and out; `ReplayWriter::Layout` is `Compact` or `Pretty`. |
| `EventJson.hpp`, `PayloadJson.hpp`, `ReplayJson.hpp` | — | The JSON encoding, validated with `json-schema-validator`. |
| `SchemaVersion.hpp` | `kReplayDocumentVersion`, `kTickEventSchemaVersion`, `kSchemaVersionKey` | The versions, named here rather than written at a call site. |
| `IMigration.hpp`, `MigrationChain.hpp` | `IMigration`, `MigrationChain` | Single-step N-to-N+1 migrations, applied until a document is current. |
| `ReplayMigrations.hpp` | `standardReplayMigrations()` | The replay document's own chain, constructed and injected. |
| `ReplayFormatError.hpp` | `ReplayFormatError` | Bad replay input. |
| `SchemaVersionError.hpp` | `SchemaVersionError` | Narrower: a document this build cannot bring to the current version. |
| `ReplayCli.hpp` | `ReplayCliOptions`, `replayCliFlags()` | The `--record <path>` / `--replay <path>` flags every app shares; the parsing is [`cli`](cli.md)'s. |
| `CommandLine.hpp`, `FlagSpec.hpp`, `CommandLineError.hpp` | — | `using` re-exports of the [`cli`](cli.md) names, kept so the old spelling still compiles. |
| `TickEventRecorder.hpp` | `TickEventRecorder` | Records what was dispatched, minus each app's self-generated events. |
| `CanvasCheck.hpp` | `CanvasCheck` | Guards a replay recorded against one canvas size from being replayed against another. |

## Depends on

[`cli`](cli.md), [`engine`](engine.md), [`event`](event.md), [`gfx`](gfx.md), [`log`](log.md), [`simulation`](simulation.md), [`time`](time.md).
`simulation` is there because `ReplaySource` implements its seam, and `gfx` for the recorded canvas size — both narrow, deliberate edges.

## Non-obvious decisions

**Only externally-supplied events are persisted.**
Anything the engine regenerates on its own — `engine.tick` above all — is never written.
Each app names its self-generated events so the recorder can drop them, and no `input.*` name may ever appear in such a list.

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

**`ReplaySource::eventsFor()` scans linearly.**
An index has deliberately not been built, because replays are not yet long enough for it to matter.
