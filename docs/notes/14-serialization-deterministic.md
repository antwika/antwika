# 14 — Serialization is itself deterministic

**Status:** not started

## Rationale/motivation

[Item 13](13-determinism-proven-by-test.md) proves that *replaying* is
deterministic. That's not quite the same claim as "serializing is
deterministic" — it's possible (in principle, if the codec were implemented
carelessly, e.g. by iterating an unordered container of sinks or event
fields) for two serializations of the identical in-memory recording to
produce different bytes while still deserializing back to equal in-memory
data. That would be a latent bug: replay files wouldn't be diffable/
cacheable/content-addressable, and it would suggest a hidden nondeterminism
that might eventually leak into state too.

## How it's satisfied

A dedicated test serializes the same `vector<TimedEvent>` twice via
`BinaryReplayWriter` and asserts the two output byte buffers are
byte-for-byte identical. Combined with
[item 09](09-serialization.md)'s codec only ever touching `std::vector`/
`std::deque` (no unordered containers, no pointer/address-derived data), this
is a cheap, high-value regression guard.

## Issues encountered

_(filled in during implementation)_
