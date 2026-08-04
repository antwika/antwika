# antwika::config

`src/libs/config/` — the versioned-JSON plumbing every application's config file shares; what a config *means* stays in the application.

## What it is for

Reading and writing an application's tuning as a versioned JSON document: the envelope a document states its magic and version in, the bounded-whole-number schema fragment a bad value is refused by, the `parse -> read version -> migrate -> validate` pipeline, and the stream and file handling around it.

It began inside [`apps/game`](../apps/game.md), as that application's `ConfigFile.cpp`, and moved here so a second application's config would not repeat the half of it that has nothing to do with a city.
The split is the one [`cli`](cli.md) draws for flags: this library owns the mechanics, and each application owns a table — its magic, its version, its migrations, its schema properties, its encode and its decode — so nothing here names an application or a field.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Format.hpp` | `Format` | One format's magic and current version, declared by the application beside its constants. |
| `ConfigDocument.hpp` | `documentSchema()` | The envelope schema: unknown members refused, magic required and pinned, version described but optional. |
| `ConfigDocument.hpp` | `wholeShape()` | A whole number bounded both ways, so a zero period or a negative cost is refused beside the parse. |
| `ConfigDocument.hpp` | `newDocument()` | A document with the magic and version stamped, ready for the caller's members. |
| `ConfigDocument.hpp` | `migrated()` | `replay::readVersionedDocument` on this module's error type: migrate, then validate, then hand back. |
| `ConfigDocument.hpp` | `memberOr<T>()` | An optional member or the caller's default, which is what makes a one-line config a one-line rebalance. |
| `ConfigDocument.hpp` | `parseConfig()` / `writeConfig()` | A document off a stream and onto one, two-space indented for diffing. |
| `ConfigDocument.hpp` | `parseConfigFile()` | A file's document, or nothing at all for a file that is not there. |
| `ConfigFormatError.hpp` | `ConfigFormatError` | The one failure category: a config document could not be read. |
| `FileFormat.hpp` | `FormatSpec` / `FileFormat<ValueT, ErrorT>` | One application's format as data -- magic, schema members, encode, decode -- with the shared read/write plumbing attached once. |
| `ConfigDocument.hpp` | `migratedAs` / `parseAs` / `parseFileAs` / `writeDocumentFileAs` | The same pipeline for a caller that reports its own error type. |

## Depends on

[`replay`](replay.md), for `MigrationChain` and `readVersionedDocument()` — [`docs/schema-versioning.md`](../../docs/schema-versioning.md) makes those the one migration mechanism for every persisted format, and this library adds a document kind rather than a second mechanism.
[`io`](io.md), for the open/flush/check discipline around a document file, stated once over there rather than a second time here.

## The two halves an application writes

An application's loader states a `FormatSpec` -- its magic and version, a `members` function adding its schema properties, an `encode`, a `decode` and a migrations factory -- and forwards its public free functions to one static `FileFormat`.
The pipeline, the envelope, the validator's construction and the file handling therefore exist once, and a new application writes only what is genuinely its own.

Its tests instantiate `ConfigFileContract` from `antwika::config::tests::conformance` -- the fourteen promises every config file makes, as one typed suite on `MessageSetCompleteness`'s pattern -- and keep only their format's own rules (poker's blind ordering, the game's per-kind cost table) as local tests.

## It reads any versioned document, not only a config

`FileFormat` takes the error type as a second parameter, defaulted to `ConfigFormatError`, because a save, an options file and a high score are read in exactly the same order and differ only in what a bad one is *called* -- and the house rule is one exception type per failure category.
`tower_defence`'s `HighScore` and `apps/game`'s options file are full `FormatSpec` ports now.
The three heaviest documents -- the game save, the companion save and the sudoku puzzle -- take the pipeline without the restructuring: they call `migratedAs`, `parseAs` and `writeConfig` directly, so the migrate-then-validate order, the JSON parse and the two-space dump are shared while their own schema and codec stay where they are.
That split is deliberate rather than unfinished: those three carry sections, migrations and a raw-grid pre-parse that a `FormatSpec`'s four function pointers do not describe well, and the duplication worth removing was the plumbing.

## Non-obvious decisions

**Every member is optional, and the library is where that stance lives.**
`memberOr()` answers the caller's default for an unstated member, so a config stating one number is a one-line rebalance rather than a restatement of every default it leaves alone — and adding a field stays additive, which is why a format built on this should stay at version 1 until a member changes meaning.
The other side of that coin is `documentSchema()` refusing unknown members: with absence meaning "default", a misspelt member silently ignored would be a rebalance that never took, so it has to be a refusal instead.

**A missing file is nothing, not an error and not a default.**
`parseConfigFile()` answers `std::nullopt` because only the application knows what its defaults *are* — the library would have to invent a document to say otherwise.
Anything wrong with a file that is there is refused rather than repaired, on the terms every persisted format here holds to.

**One `ConfigFormatError` for every application's config.**
The failure category is "a config file could not be read", and it is this module's to declare — exactly as `ReplayFormatError` covers every application's replays.
An application's save and options files keep error types of their own, because a session and a key layout are different categories of document from a tuning.

**Decode stays with the caller, and so does the validator.**
`migrated()` takes the chain and the validator rather than building them, because which migrations exist and what the current schema says are facts about one format — the library holding either would be a registry, and two formats in one process would see each other's.

## Used by

[`apps/game`](../apps/game.md) reads `config.json` through this for its `GameConfig` — see the config section of that page for what may and may not live in a game's config file.

## What is generic now, and what is still the application's

`FileFormat` left each application a trailer of its own — a static `FileFormat` built from a `FormatSpec` of the same shape, an empty `MigrationChain`, and six free functions forwarding to it — and all nine were byte-identical apart from the config type's name.
`AppConfigFile.hpp` is that trailer said once: `AppConfigFile<Spec>` holds the static format, builds the chain and answers the six calls, and the `ANTWIKA_CONFIG_FILE` macro beside it writes the `Spec` and the six functions the application's header has already promised.

The macro is there for one reason, and it is not the logic: no template can give a function in `antwika::life` the name `loadConfigFileOrDefaults`, so what a macro contributes here is names, and every line it emits is one call into the template.
It reads the `kConfigMagic` and `kConfigFormatVersion` of the namespace it is invoked in rather than taking them as arguments, so a format cannot state one thing in its header and another in its loader.
It builds the two messages a refusal carries — `antwika <app> config document` and `antwika::<app>: config JSON failed schema validation: ` — from the application's name, because nine copies of one sentence were nine chances to word it differently.

What stays with the application is what always differed: its magic and version, the schema members, the encode, the decode, and whatever rule between two members that decode refuses — poker's blind ordering, the game's household staples.
Its header stays its own too, declaring the six functions with its own documentation, since that header is the application's public face and a macro there would hide it from a reader and from generated docs alike.
The chain is the template's rather than the application's because all nine are empty for the same reason — every member is optional, so adding one is additive and the version stays at 1 — and a format that does grow a migration outgrows `AppConfigFile` and states a `FormatSpec` of its own.
That is where the documents named above stay: `AppConfigFile` is the shape a *config* has, and a save with sections, migrations and a raw-grid pre-parse is not one.
