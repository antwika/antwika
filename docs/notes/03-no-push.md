# 03 — No push to remote

**Status:** in progress (ongoing constraint, re-affirmed at [18](18-definition-of-done.md))

## Rationale/motivation

Requested explicitly: commits stay local. Pushing is a shared-state action
with a much higher blast radius than committing locally (it's visible to
others, triggers CI, etc.) and should be a separate, explicit decision made
by the repo owner, not something bundled into "implement the plan."

## How it's satisfied

Only `git commit` is used during implementation. No `git push`,
`git fetch --prune`, or remote-mutating command is run.

## Issues encountered

None yet.
