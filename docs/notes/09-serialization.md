# 09 — Serialization

**Status:** not started

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
