# Replay System — Implementation Checklist

See [`PLAN.md`](PLAN.md) for the full design/rationale. This checklist tracks
*concepts and goals*, not file-by-file steps. Every item is numbered; each
number has a correlated note in [`notes/`](notes/) documenting why it's
there, how it's satisfied, and any issues found while implementing it —
written up **as the item is implemented**, not upfront.

## Working agreement (applies to every item below)

- [ ] **01.** Tests are written alongside each item, as it's built — never
      deferred to a later "testing pass." — [notes/01-tests-alongside.md](notes/01-tests-alongside.md)
- [ ] **02.** Each completed item (or natural sub-chunk of one) gets its own
      git commit, message in [Conventional Commits](https://www.conventionalcommits.org/)
      style. Small commits, not one giant one. — [notes/02-commit-per-chunk.md](notes/02-commit-per-chunk.md)
- [ ] **03.** Nothing is pushed to any remote during implementation. — [notes/03-no-push.md](notes/03-no-push.md)

## Core concepts

- [x] **04.** Fixed simulation step. The engine advances in discrete,
      fixed-size ticks — not wall-clock time — so "what happened" is fully
      determined by "which tick, in what order," never by real elapsed
      time. — [notes/04-fixed-simulation-step.md](notes/04-fixed-simulation-step.md)
- [x] **05.** Every dispatched event is tick-stamped, automatically, in one
      place — no call site has to remember to do it, and nothing dispatched
      can slip through un-stamped. — [notes/05-automatic-tick-stamping.md](notes/05-automatic-tick-stamping.md)
- [ ] **06.** Events are extendable. Application code can define and
      dispatch its own event kinds, on top of a small set of common/built-in
      ones the engine itself provides — both travel through the exact same
      pipeline, with no special-casing between "built-in" and
      "custom." — [notes/06-extendable-events.md](notes/06-extendable-events.md)
- [x] **07.** The engine ships at least one common, built-in event (a
      per-tick "tick happened" event) so application code can react to
      simulation progress without inventing and manually dispatching its
      own. — [notes/07-builtin-common-events.md](notes/07-builtin-common-events.md)
- [ ] **08.** Recording. Every tick-stamped event that occurs — built-in or
      custom — is captured, in order, into a replayable history. Nothing
      that happens during a run is excluded from the
      recording. — [notes/08-recording.md](notes/08-recording.md)
- [x] **09.** Serialization. A recorded history can be turned into a byte
      stream and back into an equal history. The format is self-describing
      (versioned) so bad/incompatible input fails loudly and specifically
      rather than corrupting state silently. — [notes/09-serialization.md](notes/09-serialization.md)
- [ ] **10.** Replay playback uses the same code path as a live run. Loading
      a replay doesn't take a shortcut — it drives the exact same
      tick/dispatch machinery a live run uses, just sourced from the file
      instead of live input. This is what makes determinism provable rather
      than assumed. — [notes/10-replay-same-code-path.md](notes/10-replay-same-code-path.md)
- [ ] **11.** State is an application concern, not baked into the engine
      core. The engine core stays domain-agnostic; state is plain data,
      updated by folding tick-stamped events into it via the same extension
      mechanism used for recording — so "recording" and "updating state" are
      two instances of one idea, not two separate
      systems. — [notes/11-state-application-concern.md](notes/11-state-application-concern.md)
- [ ] **12.** State example in `apps/game`. A small, concrete demonstration:
      application-level state that updates both from a built-in engine event
      and from a custom application-defined event, proving the extensibility
      and state-representation ideas actually work together end to
      end. — [notes/12-state-example-apps-game.md](notes/12-state-example-apps-game.md)
- [ ] **13.** Determinism is proven, not asserted by inspection. A test runs
      the engine live, records, serializes, deserializes, replays into a
      fresh instance, and checks the resulting state — and the recorded
      event history — are exactly equal to the original
      run. — [notes/13-determinism-proven-by-test.md](notes/13-determinism-proven-by-test.md)
- [ ] **14.** Serialization is itself deterministic. Serializing the same
      recorded history twice produces byte-for-byte identical output —
      guards against accidental nondeterminism sneaking into the format
      itself (e.g. unordered iteration). — [notes/14-serialization-deterministic.md](notes/14-serialization-deterministic.md)
- [ ] **15.** Record/replay is reachable from the actual application, not
      just exercised in isolated unit tests — `apps/game` can both produce a
      replay from a live run and consume one to reproduce
      it. — [notes/15-record-replay-reachable.md](notes/15-record-replay-reachable.md)
- [ ] **16.** No RNG/PRNG. Explicitly out of scope for this pass; not even a
      placeholder/reserved field for one. — [notes/16-no-rng.md](notes/16-no-rng.md)
- [ ] **17.** Docs reflect the new capability (structure + how to use
      record/replay) once it exists. — [notes/17-docs-reflect-capability.md](notes/17-docs-reflect-capability.md)

## Definition of done

- [ ] **18.** All items above checked, `ctest` green,
      `scripts/check_unused_test_doubles.py` green, and the git history
      tells the story of how it was built (small, conventional commits)
      rather than landing as one opaque change. — [notes/18-definition-of-done.md](notes/18-definition-of-done.md)
