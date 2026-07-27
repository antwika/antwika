# 09 — Serialization

**Status:** done. `IEventCodec`/`BinaryEventCodec` (per-event: big-endian
tick, length-prefixed name/payload) plus `IReplayWriter`/`IReplayReader` and
their `BinaryReplayWriter`/`BinaryReplayReader` implementations (whole
sequence: 4-byte magic `"ARPL"`, format version, event count, then the
encoded events) landed in the `replay` lib. Bad magic, an unsupported
version, and truncated streams all throw the same specific
`ReplayFormatError` (see [PLAN.md §3.6](../PLAN.md#36-serialization-codec--writerreader-split-by-responsibility-new-replay-lib)).

## Issues encountered

PLAN.md §3.6 proposed reserving a header field for the fixed timestep `Δt`
the replay was recorded at, reasoning a replay file should be
"self-describing about its own playback rate." Implementing it surfaced that
this engine has no wall-clock playback rate to describe: `Engine::step()`
advances by a discrete `Tick`, not by a duration, and nothing anywhere reads
a `Δt` value. Adding the field would have meant carrying an unused,
unvalidated header field — the same shape of speculative addition already
rejected for RNG seeding in [item 16](16-no-rng.md). Dropped it; the header
is magic + version + event count only. If real-time pacing is ever added to
the engine, that's a deliberate future format version bump with an actual
consumer for the field, not a guess made now.

## Rationale/motivation

A recording (item [08](08-recording.md)) only becomes a "replay" once it can
leave the process and come back — as a file, or any other byte stream. The
format needs to be self-describing (versioned) so that loading a
bad/incompatible file fails loudly and specifically rather than silently
misinterpreting bytes as valid data. See
[PLAN.md §3.6](../PLAN.md#36-serialization-codec--writerreader-split-by-responsibility-new-replay-lib).

## How it's satisfied

Three split responsibilities (SRP/ISP), new `replay` lib:
- `IEventCodec`/`BinaryEventCodec` — encode/decode **one** `TimedEvent`
  to/from a stream (tick, then length-prefixed `name`, then length-prefixed
  `payload`).
- `IReplayWriter`/`BinaryReplayWriter` — write a whole sequence plus a small
  versioned header (magic bytes, format version, recorded `Δt`, event count).
- `IReplayReader`/`BinaryReplayReader` — the inverse, throwing a specific
  exception type on bad magic/version/truncated input.

I/O is `std::ostream&`/`std::istream&`, matching the existing
`StreamAppender(std::ostream&)` pattern in the `log` lib — keeps the lib
filesystem-agnostic and trivially testable via `std::stringstream`. No new
third-party dependency (see [PLAN.md §6](../PLAN.md#6-dependency-decision)).

## Issues encountered

_(filled in during implementation)_
