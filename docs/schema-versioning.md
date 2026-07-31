# Schema versioning

Every persisted document in this code base states which revision of its schema it was written against.
A reader that cannot understand a revision refuses the document instead of decoding it into something plausible and wrong.

This document is normative.
The constants it talks about live in [`antwika/replay/SchemaVersion.hpp`](../src/libs/replay/include/antwika/replay/SchemaVersion.hpp).

## The versioned documents

- The replay document `ReplayReader` and `ReplayWriter` read and write is at `kReplayDocumentVersion` (1), stated in the document's own `"version"` member.
- The tick-event schema, nested as the `items` of a replay's `events`, is at `kTickEventSchemaVersion` (1), stated once rather than in each event.
  No file carries that number: an event only ever appears inside a replay document, so the document's own `"version"` is what a reader dispatches on, and this constant is what a change to the event shape is reasoned about against.

Every app event payload schema (`life.toggle_cell`, `game.score_increment`, `task.submit`, `poker.*`, `input.*`) is validated through `parseAndValidatePayload()` and carries no version of its own, on purpose.
A payload only ever appears inside a replay document, so the document's version already says which revision of the payloads it holds, and a version member in every payload string would be one fact repeated once per event.
Changing a payload's shape is therefore a change to the replay document, and takes a replay-document bump and a migration.

## The version member

One name, `antwika::replay::kSchemaVersionKey`, for every document kind: `"version"`.
It is `"version"` rather than `"schemaVersion"` because that is what the replay format has written since it shipped, and renaming it would break every file already on disk to no end.
A document kind that has to differ may name its own key -- both `documentVersion()` and `MigrationChain` take one -- but a new format should not.

A document with no version member is version 1 (`kUnversionedDocumentVersion`).
Version 1 predates this mechanism, so a file written before it has nothing to say on the subject.
The checked-in fixtures under `src/apps/*/replays/` all state their version outright, so the absent-is-1 rule is there for a third party's older file.

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
3. Write an `IMigration` from N to N+1 and add it to the vector `standardReplayMigrations()` builds.
   It gets the parsed document and rewrites it in place, and it must not touch the version member, which `MigrationChain` stamps itself.
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
