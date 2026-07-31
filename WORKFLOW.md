# The agent loop

This document is the process an autonomous agent follows to improve this repository without a human in the loop.
It is written for an agent picking the work up cold, so it states what to read, what to do, and what "done" means, in that order.
A human reading it should be able to stop the loop at any point and find the repository in a mergeable state.

## The files the loop owns

| File | Tracked | Who writes it | What it is |
| --- | --- | --- | --- |
| `WORKFLOW.md` | yes | the orchestrator | this document: the process itself, and the standing proposals for improving it |
| `SUGGESTIONS.md` | yes | auditor agents | the ranked backlog of source-code improvements, each with a verification command |
| `LESSONS.md` | yes | every agent | what an iteration learned that the next one must not rediscover |
| `REVENUE.md` | yes | the revenue analyst | how the libraries and applications could plausibly be sold |
| `STATUS.md` | **no** | the orchestrator | the per-iteration log, newest first, deliberately untracked |
| `ISSUES.md` | yes | any agent | questions only a human can answer, parked so the loop never blocks on one |
| `IDEAS.md` | yes | any agent | candidate applications, a menu rather than a plan |

`STATUS.md` is untracked on purpose: it is operational noise about *runs*, not a statement about the code, and committing it would put a merge conflict between every pair of parallel agents.
Everything an agent *learned* belongs in `LESSONS.md`, which is tracked, so the knowledge survives even though the run log does not.

## One iteration

An iteration is one wakeup of the loop.
It has five phases and they are strictly ordered, because each one's output is the next one's input.

### 1. Orient (orchestrator, cheap — a handful of tool calls)

Read the top of `STATUS.md` for what the last iteration did and left behind, then `LESSONS.md`, then `SUGGESTIONS.md`.
Run `git worktree list` and `git status` to see what is still in flight.
Confirm `main` is clean and that nothing is half-merged.
If the previous iteration left a worktree with unmerged work, finishing that comes before starting anything new.

### 2. Plan (orchestrator)

Pick the work for this iteration from `SUGGESTIONS.md`, top-ranked first, subject to two hard rules.

- **No two agents in one iteration may touch the same file.**
  This is the whole reason parallel agents are affordable here, and it is checked by reading each suggestion's `WHERE` before dispatch rather than discovered at merge time.
- **Every task must be independently verifiable**, by a command written down in the suggestion before the agent starts.
  A task whose success cannot be stated as a command is a task for a human, and belongs in `ISSUES.md` instead.

Twelve agents is the ceiling.
Fewer is normal: the limit that binds in practice is how many genuinely disjoint suggestions the backlog holds, not the agent budget.

### 3. Fan out (up to 12 agents, in parallel)

Every implementer agent gets its own git worktree and never touches the primary checkout:

```sh
git worktree add .worktrees/<task> -b <task>
```

Each agent's brief must state, explicitly:

- the single suggestion it owns, quoted from `SUGGESTIONS.md`, and the files it may touch;
- that it must read `CLAUDE.md` and `docs/STYLE_GUIDE.md` before writing code;
- the verification command it must leave passing;
- **a time limit**, expressed as a tool-call budget and a wall-clock instruction, with the standing rule that an agent out of budget reports what it has rather than pressing on;
- the instruction to append what it learned to `LESSONS.md` **in its own worktree**, so the merge brings the lesson along with the change;
- the instruction to write findings to a file as it goes rather than holding them until its final message (see the first entry in `LESSONS.md` for why).

An agent that cannot finish must leave its worktree building and its branch committed, and say so.
A half-finished branch that compiles is recoverable by the next iteration; a broken one costs more than it saved.

### 4. Verify and merge (orchestrator, serial)

Merging is serial even though the work was parallel, because a conflict is cheaper to resolve one at a time.
For each branch, in the order the agents finished:

1. `git merge --no-ff <task>` into `main`.
2. Build and run the affected tests.
3. Only once every branch is merged, run the full suite and the three checker scripts:

```sh
cmake --build build -j24
ctest --test-dir build --output-on-failure
python3 scripts/check_unused_test_doubles.py
python3 scripts/check_one_sentence_per_line.py
python3 scripts/check_line_length.py
```

A merge that breaks the build is reverted rather than repaired in place, and the suggestion goes back on the backlog with what went wrong recorded against it.
Coverage is the one gate too slow to run every iteration; see "The coverage gate" below.

### 5. Feed back (orchestrator)

This phase is what makes the loop a loop rather than a sequence of unrelated runs, and skipping it is the failure mode to guard hardest against.

- Remove every completed suggestion from `SUGGESTIONS.md` and add the new ones the iteration's agents found.
  **The backlog must never be left with fewer than two entries**, which is the standing instruction: if the iteration emptied it, an auditor agent is dispatched to refill it before the iteration is called done.
- Append to `LESSONS.md` anything an agent learned about the *repository or the process*, as opposed to about its own change.
- Update `wiki/` for any change a reader of the wiki would otherwise be misled by, and `CLAUDE.md` for any change to an architectural rule.
  The wiki is the project's public face and drifts silently, so it is checked every iteration rather than when somebody remembers.
- Prepend the iteration's report to `STATUS.md`.
- Schedule the next wakeup.

## The coverage gate

CI requires 100% line, function and branch coverage on the GNU leg, and the coverage build is `-O0` and slow.
Running it every iteration would make the loop's cadence the coverage build's cadence, so instead:

- every implementer agent is responsible for the coverage of the lines it adds, and is told so;
- the orchestrator runs the full coverage build **every fourth iteration**, or immediately after any iteration that added a new source file;
- `docs/confirming-unreachable-branches.md` is read before any `GCOVR_EXCL_LINE` is added, and an exclusion an agent cannot justify from that document is not added at all.

## Time limits

Nothing in this loop may run unbounded.

- Implementer agents: a stated tool-call budget in the brief, typically 60, and the instruction to report at the budget rather than continue.
- Auditor agents: typically 40.
- Builds and test runs: an explicit `timeout` on the `Bash` call, 600000 ms, never the default.
- The iteration itself: if the orchestrator is still merging when the next wakeup would fire, the wakeup is pushed rather than the iteration abandoned.

## Restarting after a failure

The loop is expected to be interrupted; it is not expected to lose work.

- Every agent's branch is committed in its own worktree, so an interrupted iteration leaves recoverable state on disk rather than in a process.
- A stopped background agent can be resumed from its transcript rather than re-run from scratch, which is much cheaper — this was learned the hard way in iteration 0 and is recorded in `LESSONS.md`.
- On restart, the orchestrator's first act is phase 1, Orient, which is written to be correct whether the previous iteration finished, half-finished or never started.
- A "truly fatal failure" — the only condition that stops the loop — means the repository cannot be built from a clean checkout and no agent has been able to establish why across two consecutive iterations.
  Everything else is an iteration that ended early, and the next wakeup carries on.

## Standing proposals for improving this workflow

These are the changes to *the loop itself* that would most reduce human involvement.
They are listed here rather than in `SUGGESTIONS.md` because that file is for source code, and are ordered by how much they would help.

1. **Make the loop's own state machine a script rather than prose.**
   Everything above is instructions an agent has to follow correctly.
   A `scripts/loop_iteration.py` that performed phase 1 (orient) and phase 4 (verify and merge) mechanically would remove the two phases where an agent's mistake is most expensive, and would let the loop report a structured result instead of a written one.
2. **A pre-dispatch conflict check.**
   The "no two agents touch one file" rule is currently enforced by the orchestrator reading `WHERE` fields.
   A script that takes the selected suggestions and fails if their file sets intersect would make it mechanical.
3. **A cheap coverage signal per iteration.**
   The full gate is too slow to run every time, but coverage of *only the files an iteration touched* is affordable, and would catch a regression in the iteration that caused it rather than three later.
4. **Machine-readable suggestions.**
   `SUGGESTIONS.md` is prose with a fixed shape.
   A `suggestions.yaml` beside it — or front-matter per entry — would let the dispatch step select, check for conflicts and mark done without an agent re-parsing prose each time, and the human-readable file could be generated from it.
5. **A wiki drift check.**
   The wiki is updated by instruction, which means it is updated when an agent remembers.
   A checker that fails when a library under `src/libs/` has no `wiki/libraries/<name>.md`, or when a public header changed without its wiki page changing in the same commit, would make drift visible the way the line-length checker makes long lines visible.
6. **Let agents nominate their own successor task.**
   The most valuable suggestions in the backlog come from agents that just spent an hour in a subsystem.
   That already happens through `LESSONS.md`, but informally; making "one suggestion for the next iteration" a required field of every agent's report would make the backlog self-refilling by construction.
7. **Record the cost of each iteration in `STATUS.md`.**
   The loop currently has no notion of what an iteration cost or returned.
   Logging agents dispatched, suggestions closed and time spent would let a later iteration notice that, say, coverage work is consistently over-running its budget, which no single iteration can see.
