# antwika::io

`src/libs/io/` — opening, writing and flushing files, stated once.

## What it is for

The eight lines every module that touches a file used to write for itself: open, check the open, write, flush, check the flush.
Four modules had independently written them out — [`config`](config.md)'s document helpers, [`app`](app.md)'s `FileSnapshotStore`, [`replay`](replay.md)'s recorder and CLI, and `apps/game`'s two save wrappers — each with the same comment explaining the same subtlety, and each with its own `/dev/full` test proving it.

No module sat low enough for all of them to share.
`config` owns a copy `replay` cannot use, since `config` depends on `replay`; `app` owns a copy `config` cannot use, since `app` sits at the top of the stack.
This library is the floor under all of them: header-only, depending on nothing, knowing bytes and text and never JSON, PNG or replays — exactly as [`pattern`](pattern.md) knows nothing about music.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `File.hpp` | `Content` | Text or Bytes; Bytes is binary mode, which is what keeps a PNG a PNG on MinGW. |
| `File.hpp` | `openToReadIfPresent()` | Open to read where an absent file is a state — a first run — rather than a failure. |
| `File.hpp` | `openToReadAs<ErrorT>()` | Open to read where an absent file is a refusal, thrown as the caller's own type. |
| `File.hpp` | `openToWriteAs<ErrorT>()` | Open to write or refuse, for a caller that keeps streaming — `replay`'s recorder, a PNG writer. |
| `File.hpp` | `requireStreamTookAs<ErrorT>()` | Flush, and refuse a stream that did not take it all. |
| `File.hpp` | `writeFileAs<ErrorT>()` | The whole-file case in one call: open, write, flush, check. |

## Depends on

Nothing.

## Non-obvious decisions

**It owns no exception type, and that is the point.**
The house rule is one exception type per failure category, declared by the module that owns the failure — and a save that cannot be written is `game`'s failure, not this library's.
Every function is templated on `ErrorT`, following the shape `config::writeDocumentFileAs` and `app::FileSnapshotStore` already had, so a caller's contract ("throws `SaveFormatError`") survives the delegation unchanged.
The message text is composed here — "antwika: could not open `subject` to write: `path`" — with the caller naming only the `subject`, so every refusal in the tree reads the same way.

**The flush happens in a named function, never a destructor.**
A destructor cannot report that it failed, and a full disk fails on the flush rather than on the open — so a write nobody flushed and checked is one the filesystem may refuse in silence.
That sentence used to be a comment in four modules; `requireStreamTookAs()` is it as code, and `FileTest`'s `/dev/full` case is it as proof, once.
The suites that used to prove it per module — `game`'s saves and options, `companion`'s pet store, `tower_defence`'s score store, `replay`'s CLI — now prove only their formats and their wiring, which is what a unit test of theirs should have been testing all along.

**An absent file is two different things, so there are two open functions.**
For a config, a save or a snapshot, a missing file is a first run, and `openToReadIfPresent()` answers `nullopt` — unchecked, it would reach a parser as an empty stream, which reports "you have never had one of these" as corruption.
For a `--replay` path or an `--image` argument, a missing file is a mistyped path, and `openToReadAs<ErrorT>()` refuses it in the caller's own type.
Which of the two a given file is belongs to the caller's meaning, not to this library, which is why both exist rather than one with a flag.

**`replay` keeps its per-line flush, and only the mechanism moved.**
A recording nobody flushed is one a kill loses, so `ReplayRecorder` flushes after every line it appends — that cadence is `replay`'s contract, documented on [its page](replay.md), and this library does not know it exists.
What moved here is only the flush-then-check itself, which the recorder now reaches through `requireStreamTookAs<ReplayFormatError>()`.
