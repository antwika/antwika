# 01 — Tests written alongside

**Status:** in progress (ongoing constraint, re-affirmed at [18](18-definition-of-done.md))

## Rationale/motivation

Requested explicitly: "Make sure that you create unit tests along the way,
and verify the behavior, do not leave testing until the end." Testing after
the fact tends to bend towards whatever the code happens to do rather than
what it's supposed to do, and defers discovery of design mistakes (e.g. an
interface that's awkward to mock) until they're expensive to fix. Writing the
test for a unit right after (or before) the unit itself keeps that feedback
loop short — see [PLAN.md §8](../PLAN.md#8-process-working-agreement-for-implementation).

## How it's satisfied

Every implementation step in this project adds its test file(s) in the same
commit as the production code it covers — never a separate "add tests"
commit trailing behind. Progress is cross-checked with `ctest --test-dir
build` after each step.

## Issues encountered

None yet.
