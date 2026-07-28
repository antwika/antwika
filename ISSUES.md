# Issues found while planning the job scheduler work

Notes gathered while researching `PLAN_SCHEDULER.md` /
`PLAN_SCHEDULER_CHECKLIST.md` that are worth a maintainer's attention but
are either out of scope for that plan or broader than it.

## 1. The gcovr exclude list is duplicated outside `.github/workflows/`

`.github/workflows/build.yml`'s "Generate coverage report" step and
`.vscode/tasks.json`'s "Gcovr coverage report" task both hardcode the
same `--exclude '.*/apps/game/src/main\.cpp' --exclude
'.*/apps/life/src/main\.cpp'` pair, independently. `PLAN_SCHEDULER_
CHECKLIST.md` §15 (as instructed) only covers verifying
`.github/workflows/`, so updating `.vscode/tasks.json` to also exclude
`apps/task-worker/src/main.cpp` won't be forced by that checklist item —
it'll need to be remembered separately, or the local "Gcovr coverage
report" task's output will start including `task-worker`'s `main.cpp` in
the on-disk HTML report even after CI's badge generation correctly
excludes it. Low stakes (only affects a local, non-CI-gating report), but
easy to miss since nothing fails loudly if it's forgotten.

Longer-term, both of these — plus `build.yml`'s `expected=(...)` binary
list (see `PLAN_SCHEDULER.md` §7) — are hand-maintained lists that grow
by one line every time a library or app is added, with no automated
check that flags "you forgot to update this list" (a missing exclude
just silently changes a coverage percentage; a missing `expected` entry
just silently skips verifying that one binary exists). Not urgent, but a
candidate for a follow-up: e.g. generating the exclude list from
`glob('src/apps/*/src/main.cpp')` at gcovr-invocation time instead of
naming each app, and/or a small script that diffs `expected=(...)`
against actual CMake-installed targets. Independent of the scheduler
work — flagging here rather than folding it into that plan, which has
no reason to touch this generalization.

## 2. `apps/task-worker` is the first hyphenated app directory

Every existing `src/apps/<name>` directory is a single word (`game`,
`life`); the request explicitly names the new one `apps/task-worker`.
`PLAN_SCHEDULER.md` §4.6 works through the consequence (directory keeps
the hyphen as requested; CMake target/binary becomes `antwika_task_
worker`; C++ namespace becomes `antwika::task_worker`, since a hyphen
isn't legal there) rather than silently picking one and moving on, but
it's worth a maintainer's explicit sign-off before implementation
starts, since renaming a directory after code exists under it (headers,
namespace, includes, CMake target, CI's `expected=(...)` list, this
plan's own file layout) is far more churn than deciding it up front. The
alternative is dropping the hyphen (`apps/taskworker`) for full
consistency with `game`/`life` — purely a naming preference, not a
technical constraint either way.
