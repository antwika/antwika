# antwika::replay

`src/libs/replay/` — the file format, the migrations, and the CLI flags.

## What it is for

Recording a run and reading one back.
Running the loop is [`simulation`](simulation.md)'s, and this library depends on it: `ReplaySource` is a `simulation::ITickEventSource`, and a live run needs nothing that is here.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `ReplaySource.hpp` | `ReplaySource` | The `simulation::ITickEventSource` that serves events out of a loaded `ReplayDocument`. |
| `ReplayDocument.hpp` | `ReplayDocument` | The in-memory form of a replay file: its records and its header's canvas. |
| `ReplayHeader.hpp` | `ReplayHeader` | The first line: the version its records are at, and the canvas the run laid its input out against. |
| `ReplayReader.hpp` / `ReplayWriter.hpp` | `ReplayReader`, `ReplayWriter` | JSON Lines in and out; the writer has `writeHeader()` and `writeRecord()` as well as a whole-recording `write()`. |
| `ReplayRecorder.hpp` | `ReplayRecorder` | The `event::ITickEventSink` a `--record` run dispatches into: one line appended and flushed per event. |
| `EventJson.hpp`, `PayloadJson.hpp`, `ReplayJson.hpp` | — | The JSON encoding; `src/EventSchema.cpp` describes one record, and header and records alike are validated with `json-schema-validator`. |
| `SchemaVersion.hpp` | `kReplayDocumentVersion`, `kTickEventSchemaVersion`, `kSchemaVersionKey` | The versions, named here rather than written at a call site. |
| `IMigration.hpp`, `MigrationChain.hpp` | `IMigration`, `MigrationChain` | Single-step N-to-N+1 migrations, applied until a document is current. |
| `ReplayMigrations.hpp` | `standardReplayMigrations()` | The replay record's own chain, constructed and injected. |
| `ReplayFormatError.hpp` | `ReplayFormatError` | Bad replay input. |
| `SchemaVersionError.hpp` | `SchemaVersionError` | Narrower: a document this build cannot bring to the current version. |
| `ReplayCli.hpp` | `ReplayCliOptions`, `replayCliFlags()`, `openReplayFile()`, `saveReplayFile()`, `loadReplayFile()` | The `--record <path>` / `--replay <path>` flags every app's `main.cpp` shares, and the file ends of them; the parsing itself is [`cli`](cli.md)'s. |
| `CanvasCheck.hpp` | `CanvasCheck` | Guards a replay recorded against one canvas size from being replayed against another. |

[`event`](event.md)'s `TickEventRecorder` is the in-memory one, for a caller that wants a run's events rather than a file of them; `ReplayRecorder` here is what a `--record` run uses, and `app::runRecorded()` builds one.
`antwika/replay/CommandLine.hpp`, `FlagSpec.hpp` and `CommandLineError.hpp` were `using` re-exports of the [`cli`](cli.md) names, kept while callers moved; they are gone, so `antwika::cli::` is the only spelling of those types.

## Depends on

[`cli`](cli.md), [`engine`](engine.md), [`event`](event.md), [`gfx`](gfx.md), [`io`](io.md), [`log`](log.md), [`simulation`](simulation.md), [`time`](time.md).
`simulation` is there because `ReplaySource` implements its seam, and `gfx` for the recorded canvas size — both narrow, deliberate edges.

## Non-obvious decisions

**Only externally-supplied events are persisted.**
Anything the engine regenerates deterministically on its own — `engine.tick` above all — is never written into a replay.
What lands in a recording is decided by where the recorder sits rather than by a list of names it skips: [`event`](event.md)'s `TickEventRecorder` records unconditionally, so what it sees is exactly what an `ITickEventSource` supplied for that tick, and never `engine.tick` nor anything a sink derives further down the tick path.

**A replay is JSON Lines, because a replay is a log.**
One JSON value a line: a header first, then one record for every recorded event.
The shape before it was one document holding an `"events"` array, which encoded a log inside a snapshot — nothing could be written until the run had ended, and several applications have no end of their own, so `Ctrl+C` saved nothing at all.
`ReplayRecorder` appends and flushes a line the moment an event arrives, so a run killed part-way keeps everything up to the kill and leaves a file that replays.

Flushed, not synced: the bytes reach the operating system, which is what makes them survive the process dying.
An `fsync` per event would buy a recording nothing, since there is no recording without a process to have made it.

**There is deliberately no layout to choose.**
A record has to fit on one line for a reader to find where it ends, so `Layout::Pretty` could not survive — and one event a line is already the diffable form a pretty-printed array was asked for.
The checked-in demos under `src/apps/*/replays/` are `demo.jsonl`, and were converted by driving each application with `--replay` over the old file and `--record` writing what it actually dispatched, rather than by editing them.

**A `--record` path is opened before the first tick**, not after the last, since that is when a recording starts being written.
So a mistyped path is answered straight away instead of after the session it then had nowhere to keep.

**Every persisted schema states its version.**
A replay states its once, in its header line, and never in a record: a record repeats thousands of times and cannot disagree with the header that opens it.

Reading is `parse → read version → migrate → validate → decode`, per record, at the version the header stated — `readVersionedRecord()` and `MigrationChain::migrateFrom()` are that substitution.
Validating *after* migrating is what lets exactly one schema exist rather than one per revision.
The header itself is never migrated, because it is what states the version; its schema therefore describes every version at once, and the header may only ever change additively.
A file with no version member is version 1, which is what every replay written before this says.

**A version 1 whole-document replay still loads**, and is read by splitting rather than by a decoder of its own: such a document minus its `"events"` is exactly a header, and each element of that array is exactly a record.
Which shape a file is is decided from the members of the first JSON value in it, rather than from the first non-space character as `sudoku::PuzzleFile` decides its two — both replay shapes open with `{`.

**Two rules a whole document used to state by being one** are now checks in `replayRecordsFromJson()`: a recording's ticks never go backwards, and a file holds one header and it is the first line.

**A torn last line is dropped rather than refused.**
The newline that ends a record is its commit marker, so a final line without one that will not parse is a write the kill tore off part-way; refusing it would throw away the recording that came before it, which is the whole failure appending exists to remove.

`MigrationChain` applies single-step migrations so the number of them stays linear in bumps rather than quadratic, and it is generic over an `nlohmann::json` and a version key — so a save file uses the same mechanism with its own list and its own current version.
Chains are constructed and injected, never registered globally.
A chain that cannot say what it does is refused when it is built rather than when somebody's file is being loaded: a migration that is not a single step, and two migrations reading the same version, both throw `SchemaVersionError` from the constructor — the lookup takes the first reader of a version and never looks further, so a second one would be applied by nothing and reported by nothing either.

**A schema fragment states the bound its decode has.**
`canvas.width` and `canvas.height` are described with `boundedCountShape(uint32 max)` rather than the unbounded `countShape()`, because they are decoded with `get<std::uint32_t>()` and nlohmann takes the low bytes of anything wider without a word — an unbounded shape let a hand-edited `4294967297` validate and read back as `1`.
Tightening a constraint is normally a breaking change, but this one refuses only documents that never decoded to what they said in the first place, and the header schema is the one that cannot be bumped anyway — see [`docs/schema-versioning.md`](../../docs/schema-versioning.md).

**`ReplaySource`'s constructor sort is for scripted event vectors, never for files.**
A recording whose ticks go backwards is refused outright by `replayRecordsFromJson()`, since that is two runs interleaved or a hand edit that moved a line; the sort exists so a test or an app can hand `ReplaySource` a vector it built in whatever order it liked.

See [`docs/schema-versioning.md`](../../docs/schema-versioning.md) for what counts as a breaking change.

**`ReplaySource::eventsFor()` scans linearly.**
An index has deliberately not been built, because replays are not yet long enough for it to matter.
