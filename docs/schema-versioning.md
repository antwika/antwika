# Schema versioning

Every persisted document in this code base states which revision of its schema it was written against.
A reader that cannot understand a revision refuses the document instead of decoding it into something plausible and wrong.

This document is normative.
The constants it talks about live in [`antwika/replay/SchemaVersion.hpp`](../src/libs/replay/include/antwika/replay/SchemaVersion.hpp).

## The versioned documents

- The replay `ReplayReader` and `ReplayWriter` read and write is at `kReplayDocumentVersion` (2), stated in the `"version"` member of the file's header line.
- The tick-event schema, which is the shape of one record of a replay, is at `kTickEventSchemaVersion` (1), stated once rather than in each record.
  No file carries that number: a record only ever appears inside a replay, so the header's own `"version"` is what a reader dispatches on, and this constant is what a change to the record shape is reasoned about against.

Every app event payload schema (`life.toggle_cell`, `game.score_increment`, `task.submit`, `poker.*`, `input.*`) is validated through `parseAndValidatePayload()` and carries no version of its own, on purpose.
A payload only ever appears inside a replay, so the file's version already says which revision of the payloads it holds, and a version member in every payload string would be one fact repeated once per event.
Changing a payload's shape is therefore a change to the replay format, and takes a replay bump and a migration.

## The version member

One name, `antwika::replay::kSchemaVersionKey`, for every document kind: `"version"`.
It is `"version"` rather than `"schemaVersion"` because that is what the replay format has written since it shipped, and renaming it would break every file already on disk to no end.
A document kind that has to differ may name its own key -- both `documentVersion()` and `MigrationChain` take one -- but a new format should not.

A document with no version member is version 1 (`kUnversionedDocumentVersion`).
Version 1 predates this mechanism, so a file written before it has nothing to say on the subject.
The checked-in fixtures under `src/apps/*/replays/` all state their version outright, so the absent-is-1 rule is there for a third party's older file.

## Snapshots and logs

Every rule above applies to a **snapshot**: one document, read whole, validated whole, decoded whole.
The game save, the companion save, the high score, the options file and the sudoku puzzle are all snapshots, and nothing in this section changes anything about them.

A **log** is the other kind, and there is one: a replay.
It is a sequence of records that only ever grows, and it is written as JSON Lines -- one JSON value a line, a header first and one record for every recorded event after it -- so that a record can be appended and flushed the instant its event happens rather than after the run that produced it has ended.
That distinction is worth stating because it decides where a version lives, and the rest of this section is the replay-specific consequences of it.

A snapshot has cross-references a whole-document schema validates as one unit, and JSON Lines buys it nothing while costing it that.
So the choice is not a matter of taste: a format is JSON Lines when it is a log and one document when it is not.

### Where a log states its version

Once, in its header line, and never in a record.
A record repeats thousands of times in one file and cannot disagree with the header that opens it, and a version per record would be one fact written down once a line.

**The header is never migrated.**
It is what states the version, so every build that will ever read the file has to understand it as it stands, before anything has been brought anywhere.
That makes the header the one schema in this code base describing every version at once rather than only the current one, and it is why the header may only ever change *additively* -- a new member there is optional, or it is a new format.
Everything that can change breakingly lives in the records.

### Reading order, per record

```
parse header -> read version -> validate header -> decode header
parse record -> migrate from that version -> validate record -> decode
```

The five stages are the same ones, and what changed is the unit they apply to: `readVersionedRecord()` is `readVersionedDocument()` with the version supplied by the header instead of read from the value in hand, and `MigrationChain::migrateFrom()` is `migrate()` with the same substitution.
Migrations stay single-step and injected -- `standardReplayMigrations()` is the replay's own chain, and what it migrates is a record.

A record carries no version member going in and none coming out, so the chain stamps nothing; `migrate()` still stamps a snapshot's, since a snapshot is asked its own version.

### What a per-line schema cannot say

A whole-document schema stated some things by being one document: every record was inside one array, so a reader that got the document got all of them, in order, once.
A file of lines has no such shape, so those rules are checks in `replayRecordsFromJson()` instead, with a test each:

- **Ticks never go backwards.** A recorder only ever appends, so a file whose ticks descend is two runs interleaved or a hand edit that moved a line, and sorting it back into shape would replay a session nobody had.
- **A file holds one header, and it is the first line.** Two recordings appended to one file would otherwise replay as a single session with the second one's ticks starting over.

A rule that spans records belongs there rather than in a schema, and adding one to the format means adding one there.

### A torn last line

The newline that ends a record is its commit marker.
A last line without one that will not parse is a write the kill that ended the run tore off part-way, and it is dropped rather than thrown on -- refusing it would throw away the whole recording that came before it, which is the failure appending exists to remove.
A line that will not parse anywhere else is a malformed file, and the reader says which line.

### Reading a version 1 replay

A version 1 replay is one JSON object holding its entire event log in an `"events"` array, and this build still reads one.
Which shape a file is is decided from the first JSON value in it: a header carries every other member a version 1 document did and never `"events"`, so a value that has one is a whole document.
That is `sudoku::PuzzleFile`'s trick one level up -- it decides from the first non-space character, which cannot work here because both shapes open with `{`.

The two converge rather than being decoded twice: a version 1 document minus its `"events"` is exactly a header, and each element of that array is exactly a record, so both go through the same pipeline into the same `ReplayDocument`.
Version 1 records reach version 2 through an identity migration, which exists because a chain refuses a gap: what changed at that bump is how records are framed in a file, not what a record is, and saying so in a step somebody wrote down and tested is better than a version nobody accounted for.

## When to bump

A change is additive when every document valid under version N is still valid under N+1 and still means the same thing.
Adding an optional member is the usual case -- `"canvas"` was one, and did not bump the version.
An additive change needs no bump.

A change is breaking when a document written under N is no longer valid, or is still valid but means something else.
That covers renaming or removing a member, making an optional member required, tightening a constraint (a narrower type, a new `minimum`, a smaller enum), and reinterpreting an existing value.

A breaking change needs a bump, and a bump needs a migration.

## Bumping, step by step

1. Increment `kReplayDocumentVersion` (or your own document's constant) by one.
   There is no minor number: a reader either understands a revision or it does not, and half-understanding is the failure this mechanism exists to prevent.
2. Change the schema in `ReplayJson.cpp` to describe the new shape.
   Only ever one schema exists, for the current version, because the reader migrates before it validates and no older schema has to be kept.
   The replay header's is the one exception, for the reason given under [Snapshots and logs](#snapshots-and-logs): it describes every version, so a breaking change to a header is not a bump but a new format.
3. Write an `IMigration` from N to N+1 and add it to the vector `standardReplayMigrations()` builds.
   It gets the parsed document -- or, for a replay, one parsed record -- and rewrites it in place, and it must not touch the version member, which `MigrationChain` stamps itself.
   Write the step even when nothing about the shape changed: a chain refuses a gap, and an identity migration is how a bump that moved something else says so.
4. Add a test that loads a literal version-N document and asserts what it becomes.
   A migration is the only thing standing between a released build and somebody's saved file, so it is tested against a document written out by hand rather than one this build produced.

Migrations are single-step, N to N+1, and chained.
Writing them as N-to-current instead would mean a new migration for every older version at every bump -- quadratic, and every one of them another place to get it wrong.

## Reading order

    parse -> read version -> migrate to current -> validate -> decode

Validating after migrating is what lets one schema exist rather than one per revision.
Reading the version first is what stops a document from a newer build being decoded on the strength of happening to satisfy today's schema.

## Failures

`SchemaVersionError` (a `ReplayFormatError`) covers everything about a version: a version member that is not a whole number, a version newer than this build knows, and a gap in the migration chain.
All three mean one thing to a caller, that this document cannot be brought to a shape this build understands, and the message always names the version found and what this build supports.
Anything else wrong with a document is a plain `ReplayFormatError`.

## Using the migrator for a save file

`MigrationChain` operates on an `nlohmann::json` and a version key, and knows nothing about replays.
A save file plugs in like this:

```cpp
inline constexpr std::uint32_t kSaveVersion = 3;

std::vector<std::shared_ptr<const IMigration>> migrations;
migrations.push_back(std::make_shared<SaveV1ToV2>());
migrations.push_back(std::make_shared<SaveV2ToV3>());

const MigrationChain chain(std::move(migrations), kSaveVersion);
chain.migrate(parsed);   // throws SchemaVersionError if it cannot
```

The chain is constructed and injected, so there is no registry and no global state, and two document kinds in one process cannot see each other's migrations.
