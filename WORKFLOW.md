# The agent loop

This document is the process an autonomous agent follows to improve this repository without a human in the loop.
It is written for an agent picking the work up cold, so it states what to read, what to do, and what "done" means, in that order.
A human reading it should be able to stop the loop at any point and find the repository in a mergeable state.

It used to be backed by a set of tracked bookkeeping files: a ranked backlog, a lessons file, a revenue analysis, a parked-questions file and an ideas menu.
They are gone, and what was worth keeping out of them has moved into this document, into `CLAUDE.md` or into the code.
A backlog file is a second place for work to live, it goes stale between iterations, and keeping it honest cost more attention than reading the tree does.

`STATUS.md` remains, untracked, as the per-iteration run log.
It is untracked on purpose: it is operational noise about *runs* rather than a statement about the code, and committing it would put a merge conflict between every pair of parallel agents.

## Where the work comes from

An iteration finds its own work rather than reading it off a list.
In rough order of how much a candidate is worth:

- **Anything left in flight.** `git worktree list` and `git status` first, every time.
  A branch from a previous iteration that is committed but unmerged is finished before anything new is started.
- **What the tree says about itself.** `CLAUDE.md` is authoritative on intent and occasionally stale on fact, so a rule naming a symbol that no longer exists is real work -- and finding one means verifying the symbol before building on the rule.
  When the documentation and the tree disagree, the tree is what runs, but the *intent* in `CLAUDE.md` is still the thing to preserve: the fix is usually to re-word the rule against the mechanism that replaced it rather than to drop the rule.
  `docs/` holds only documents that are still normative, so a plan document whose work has shipped should already have been deleted.
  `wiki/` is the public face and drifts silently, so it is checked against the tree every iteration rather than when somebody remembers.
- **What the gates say.** The four checker scripts, the coverage gate, and CI.
- **An audit of one area.** A read-only agent over one subsystem, reporting what it found, is how an otherwise empty iteration finds work -- and its findings are used *in that iteration* rather than written down for a later one.

## One iteration

An iteration is one wakeup of the loop.
It has four phases and they are strictly ordered, because each one's output is the next one's input.

### 1. Orient (orchestrator, cheap -- a handful of tool calls)

Read the top of `STATUS.md` for what the last iteration did and left behind.
Run `git worktree list` and `git status` to see what is still in flight.
Confirm `main` is clean and that nothing is half-merged.
If the previous iteration left a worktree with unmerged work, finishing that comes before starting anything new.

### 2. Plan (orchestrator)

Pick this iteration's work, subject to two hard rules.

- **No two agents in one iteration may touch the same file.**
  This is the whole reason parallel agents are affordable here, and it is checked by writing each task's file set down before dispatch rather than discovered at merge time.
- **Every task must be independently verifiable**, by a command written down before the agent starts.
  A task whose success cannot be stated as a command is a task for a human, and it is raised with the human directly rather than parked in a file for nobody to read.

Twelve agents is the ceiling.
Fewer is normal: the limit that binds in practice is how many genuinely disjoint tasks the tree offers, not the agent budget.

### 3. Fan out (up to 12 agents, in parallel)

Every implementer agent gets its own git worktree and never touches the primary checkout:

```sh
git worktree add .worktrees/<task> -b <task>
```

Each agent's brief must state, explicitly:

- the single task it owns and the files it may touch;
- that it must read `CLAUDE.md` and `docs/STYLE_GUIDE.md` before writing code;
- the verification command it must leave passing;
- **a time limit**, expressed as a tool-call budget and a wall-clock instruction, with the standing rule that an agent out of budget reports what it has rather than pressing on;
- the instruction to write a running report to a scratch file **outside the repository**, updated as it works rather than held until its final message, so an interrupted agent still leaves its findings behind.

That last one was learned expensively.
A background agent dies when the process that launched it exits, and it reports nothing: three audit agents were lost that way after doing real work, and a final message is exactly what an interrupt destroys where a file survives.
All three were recovered by resuming them from their transcripts with an instruction to stop investigating, spend at most three more tool calls, and report what they had already verified -- which is far cheaper than re-running an audit.
An agent that fans out to its own sub-agents inherits that problem and multiplies it, so prefer one agent per area, dispatched by the orchestrator, over an agent that dispatches its own.

An agent that cannot finish must leave its worktree building and its branch committed, and say so.
A half-finished branch that compiles is recoverable by the next iteration; a broken one costs more than it saved.

### 4. Verify and merge (orchestrator, serial)

Merging is serial even though the work was parallel, because a conflict is cheaper to resolve one at a time.
For each branch, in the order the agents finished:

1. `git merge --no-ff <task>` into `main`.
2. Build and run the affected tests.
3. Only once every branch is merged, run the four checker scripts and then the full suite:

```sh
python3 scripts/check_unused_test_doubles.py
python3 scripts/check_one_sentence_per_line.py
python3 scripts/check_line_length.py
python3 scripts/check_readme_modules.py
cmake --build build -j24
SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build --output-on-failure
```

The checkers come first because they are the first thing CI runs, they are the most common failure -- an 80-character line, a comment holding two sentences, or a module README does not list -- and all four finish in under a second.
Catching a style violation in one second beats catching it after a twenty-minute matrix.

**Those two environment variables are not optional.**
A bare `ctest --test-dir build` reports dozens of failures on a completely healthy tree, because the pre-configured `build/` is an SDL3 configuration in a container with no display and no audio device, so every SDL conformance case fails with "could not create a renderer".
It is the single most expensive trap here for an autonomous agent, because it looks exactly like a regression the agent just caused.
`xvfb-run -a` works too and is what CI uses for the gfx and input legs, but the sound leg runs under `SDL_AUDIO_DRIVER=dummy` with no display *on purpose*: a sound backend that needed a display would have quietly taken a dependency on video, and Xvfb is exactly what would hide that.

Note also that `build/` need not be the configuration `CLAUDE.md`'s default commands describe.
Which backends its cache holds is whatever the untracked `.vscode/<subsystem>-backend` files select, because `scripts/build.sh` reads those on every build -- and a missing file means `null`.
An agent assuming either configuration will misread which targets exist; `build/CMakeCache.txt`'s `ANTWIKA_*_BACKEND` entries are the machine's actual answer.

A merge that breaks the build is reverted rather than repaired in place, and what went wrong is recorded in `STATUS.md` against the task.

Then, before the iteration is called done:

- Update `wiki/` for any change a reader of the wiki would otherwise be misled by, and `CLAUDE.md` for any change to an architectural rule.
- Prepend the iteration's report to `STATUS.md`.
- Schedule the next wakeup.

## The coverage gate

CI requires 100% line, function and branch coverage on the GNU leg, and the coverage build is `-O0` and slow.
`build-coverage/` does not exist in a fresh checkout, so "check coverage" means a full `conan install` plus a configure and build from scratch.
Running that every iteration would make the loop's cadence the coverage build's cadence, so instead:

- every implementer agent is responsible for the coverage of the lines it adds, and is told so;
- the orchestrator runs the full coverage build **every fourth iteration**, or immediately after any iteration that added a new source file;
- `docs/confirming-unreachable-branches.md` is read before any `GCOVR_EXCL_LINE` is added, and an exclusion an agent cannot justify from that document is not added at all.

The gate is `scripts/check_full_coverage.py --summary coverage-summary.json`, which fails unless line, function *and* branch percentages are each exactly 100.
It is enforced on the GNU leg only; LLVM produces a report and a badge but does not enforce.

The coverage build is roughly 20x slower than release, so a test that repeats expensive work is affordable at `-O2` and not at `-O0`.
`LevelGeneratorTest` is the worked example of the split: eight seeds under instrumentation, forty in the release-build CI soak.
Any new wide or soak test must follow that pattern or it will blow up the coverage legs.

## Worktrees

`git worktree list` and `ls .worktrees/` disagree, and badly.
Directories exist under `.worktrees/` whose `.git` file points at a gitdir that no longer exists, so `git -C <dir> status` fails **silently and empty** rather than erroring -- meaning a sweep over `.worktrees/*/` gets a false "nothing uncommitted" from every one of them.
They also hold double-digit gigabytes, and creating a new worktree whose name collides with an orphan fails on a non-empty directory.

**Check the name you intend to use with `ls .worktrees/` before `git worktree add`.**
`git worktree list` is not the check.

## Time limits

Nothing in this loop may run unbounded.

- Implementer agents: a stated tool-call budget in the brief, typically 60, and the instruction to report at the budget rather than continue.
- Auditor agents: typically 40.
- Builds and test runs: an explicit `timeout` on the `Bash` call, 600000 ms, never the default.
- The iteration itself: if the orchestrator is still merging when the next wakeup would fire, the wakeup is pushed rather than the iteration abandoned.

## Restarting after a failure

The loop is expected to be interrupted; it is not expected to lose work.

- Every agent's branch is committed in its own worktree, so an interrupted iteration leaves recoverable state on disk rather than in a process.
- A stopped background agent can be resumed from its transcript rather than re-run from scratch.
- On restart, the orchestrator's first act is phase 1, Orient, which is written to be correct whether the previous iteration finished, half-finished or never started.
- A "truly fatal failure" -- the only condition that stops the loop -- means the repository cannot be built from a clean checkout and no agent has been able to establish why across two consecutive iterations.
  Everything else is an iteration that ended early, and the next wakeup carries on.

## Standing proposals for improving this workflow

These are the changes to *the loop itself* that would most reduce human involvement, ordered by how much they would help.

1. **Make the loop's own state machine a script rather than prose.**
   Everything above is instructions an agent has to follow correctly.
   A `scripts/loop_iteration.py` that performed phase 1 (orient) and phase 4 (verify and merge) mechanically would remove the two phases where an agent's mistake is most expensive, and would let the loop report a structured result instead of a written one.
2. **A pre-dispatch conflict check.**
   The "no two agents touch one file" rule is currently enforced by the orchestrator holding each task's file set in its head.
   A script that takes the selected tasks and fails if their file sets intersect would make it mechanical.
3. **A cheap coverage signal per iteration.**
   The full gate is too slow to run every time, but coverage of *only the files an iteration touched* is affordable, and would catch a regression in the iteration that caused it rather than three later.
4. **A wiki drift check.**
   The wiki is updated by instruction, which means it is updated when an agent remembers.
   A checker that fails when a library under `src/libs/` has no `wiki/libraries/<name>.md`, or when a public header changed without its wiki page changing in the same commit, would make drift visible the way the line-length checker makes long lines visible.
5. **A worktree reaper.**
   The orphaned directories described above are found by hand today.
   A script that lists `.worktrees/*/` against `git worktree list`, reports the difference and its size, and removes an orphan on request would stop that recurring.
6. **Record the cost of each iteration in `STATUS.md`.**
   The loop currently has no notion of what an iteration cost or returned.
   Logging agents dispatched, tasks closed and time spent would let a later iteration notice that, say, coverage work is consistently over-running its budget, which no single iteration can see.
