# 16 — No RNG/PRNG

**Status:** not started

## Rationale/motivation

Explicitly requested to be out of scope for this pass, including not
reserving a header field for a future seed. The temptation, when designing a
replay format, is to "future proof" it by reserving space for randomness
seeding — but that's guessing at a future design (how would randomness be
scoped? per-run? per-reducer? re-seeded per tick?) before any gameplay code
actually needs it. Deferring the decision entirely, rather than half-deciding
it now, keeps the replay format's first version minimal and avoids carrying
an unused/unvalidated field.

## How it's satisfied

Nothing in this implementation reads from `<random>`, `std::random_device`,
or any other entropy source. No `seed` field exists anywhere in `Event`,
`TimedEvent`, or the replay file header. If/when gameplay code needs
determinism-sensitive randomness, that's a deliberate future replay-format
version bump, not something pre-guessed here.

## Issues encountered

None — verified by inspection at the end of implementation (see
[item 18](18-definition-of-done.md)): no `<random>` include anywhere under
`src/`.
