# Lessons

What agents working in this repository have learned, so the next one does not pay to learn it again.
This file is tracked; `STATUS.md` is not.
The division is deliberate: a *run* is noise, and a *lesson* is knowledge, and only one of them is worth a merge conflict.

Append to the bottom of the relevant section rather than rewriting it, and date nothing — an entry that stops being true should be deleted or corrected, not annotated with a history.

## Running the tests

**A bare `ctest --test-dir build` shows 56 failures on a perfectly healthy tree.**
This is the single most expensive trap in the repository for an autonomous agent, because it looks exactly like a regression the agent just caused.
The pre-configured `build/` is an SDL3 build running in a container with no display and no audio device, so every `Sdl3/GfxBackendConformance.*`, `Sdl3RendererTest.*` and `Sdl3/SoundBackendConformance.*` case fails with "could not create a renderer: Couldn't find matching render driver".
Always run:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=dummy ctest --test-dir build --output-on-failure
```

That is **3029 passed, 0 failed, in about six seconds**.
`xvfb-run -a` works too and is what CI uses for the gfx and input conformance legs, but the sound leg is run under `SDL_AUDIO_DRIVER=dummy` with no display *on purpose* — a sound backend that needed a display would have quietly taken a dependency on video, and Xvfb is exactly what would hide that.

**The pre-configured `build/` is not the configuration CLAUDE.md's default commands describe.**
Its cache holds `ANTWIKA_GFX_BACKEND=sdl3`, `ANTWIKA_INPUT_BACKEND=sdl3`, `ANTWIKA_SOUND_BACKEND=sdl3`, because the untracked `.vscode/gfx-backend` and `.vscode/sound-backend` files select sdl3 and `scripts/build.sh` reads them on every build.
An agent assuming a `null` build will misread which targets exist and wonder why SDL3 conformance tests are present at all.

**Run the checker scripts before running any build.**
They are the first thing CI runs, they are the most common failure (an 80-character line, or a comment holding two sentences), and all three finish in under a second:

```sh
python3 scripts/check_unused_test_doubles.py
python3 scripts/check_one_sentence_per_line.py
python3 scripts/check_line_length.py
```

CI also runs `python3 scripts/generate_poker_atlas.py --check` and the six self-tests under `scripts/tests/`.
Catching a style violation in one second beats catching it after a twenty-minute matrix.

## Coverage

**`build-coverage/` does not exist in a fresh checkout**, so any request to "check coverage" means a full `conan install` plus a `-O0` configure and build from scratch.
That is expensive, which is why `WORKFLOW.md` runs the gate every fourth iteration rather than every one.

The gate is `scripts/check_full_coverage.py --summary coverage-summary.json`, which fails unless line, function *and* branch percentages are each exactly 100, and it is enforced on the **GNU leg only** — LLVM produces a report and a badge but does not enforce.

**A stale `coverage-summary.json` sits in `.worktrees/coverage-gate`.**
Feeding it to the checker reports a pass that measures nothing current.

**The coverage build is roughly 20x slower than release**, so a test that repeats expensive work is affordable at `-O2` and not at `-O0`.
`LevelGeneratorTest` is the worked example of the split: eight seeds under instrumentation, forty in the release-build CI soak.
Any new wide or soak test must follow that pattern or it will blow up the coverage legs.

## Worktrees

**`git worktree list` and `ls .worktrees/` disagree, badly.**
Four worktrees are registered; twenty directories exist.
The seventeen extras have a `.git` file pointing at a gitdir that no longer exists, so `git -C <dir> status` fails **silently and empty** rather than erroring — meaning a sweep over `.worktrees/*/` gets a false "nothing uncommitted" from every one of them.
They also hold 12-13 GB, and creating a new worktree whose name collides with an orphan fails on a non-empty directory.

**Check the name you intend to use before `git worktree add`.**
`ls .worktrees/` is the check; `git worktree list` is not.

**Only `port/i18n` holds unmerged local work.**
Every other local branch, including all three registered worktrees' branches, is already merged into `main`.

## Running agents

**A background agent dies when the process that launched it exits, and it reports nothing.**
Three audit agents were lost this way in iteration 0, each after doing real work — 95-126 KB of transcript apiece.
Two consequences, both now standing rules:

- **An agent must write its findings to a file as it goes**, not hold them for its final message.
  A final message is lost on interrupt; a file in the worktree is not.
  This is why every implementer brief says to append to `LESSONS.md` in its own worktree rather than to report the lesson at the end.
- **A stopped agent can be resumed from its transcript** by sending it a message, and that is far cheaper than re-running the audit.
  The recovery instruction that worked was: stop investigating, spend at most three more tool calls, report what you already verified, and mark anything unverified as such.
  All three agents returned complete deliverables this way.

**An agent that fans out to its own sub-agents inherits this problem and multiplies it.**
The iteration-0 source auditor dispatched three Explore agents of its own; none of them survived, and their areas — `sound`/`input`/`backends`, `ui`/`gfx`/`ttf`, and the six apps — still have had no audit pass at all.
Prefer one agent per area, dispatched by the orchestrator, over an agent that dispatches its own.

## The documentation is not always the code

**CLAUDE.md is authoritative on intent and occasionally stale on fact.**
Two instances found in iteration 0, both now carried as backlog entries:

- It states a rule about `kSelfGeneratedEventNames`, an identifier that returns **zero** hits across `src/`.
- It lists `wfc::AdjacencyConstraint` as having no application, while it is the backbone of both level generators.

Verify a symbol exists before building on a rule that names it.
When the documentation and the tree disagree, the tree is what runs — but the *intent* in CLAUDE.md is still the thing to preserve, so the fix is usually to re-word the rule against the mechanism that replaced it, not to drop the rule.
