# 17 — Docs reflect the new capability

**Status:** done. `README.md`'s project-structure tree now lists `replay/`
and `docs/`; a new "Replays" section explains the `--record`/`--replay`
flags and links to `docs/PLAN.md` and the relevant `docs/notes/` files for
anyone extending the game with new state or event kinds.

## Rationale/motivation

A feature that isn't documented in the project's own README is effectively
undiscoverable to the next person (including future-us) who opens the repo
without having read this planning doc.

## How it's satisfied

`README.md`'s project-structure tree is updated to include the new `replay`
lib and the `docs/` directory; a short "Replays" section explains the
`--record`/`--replay` flags and links to `docs/PLAN.md`/`docs/CHECKLIST.md`
for anyone extending the game with new event kinds.

## Issues encountered

_(filled in during implementation)_
