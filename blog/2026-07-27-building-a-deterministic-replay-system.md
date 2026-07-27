# Building a deterministic replay system for a fixed-timestep engine

*2026-07-27*

Antwika is a small C++23 game engine — until this change, a fairly bare skeleton: a logger, an event queue, and an `Engine::start()` that logged a line and drained whatever was sitting in the queue.
This post is about turning that skeleton into something that can record a run and reload it later to deterministically reproduce the exact same result — and about a real bug that a test caught before it ever shipped.

It's written from the git history of the branch (17 commits, `docs: move planning docs...` through the final round of coverage fixes), which is a reasonably honest record of how the thinking actually evolved, including the part where an assumption turned out to be wrong.

## Requirements

For anyone extending this system later, here's the brief in its final, distilled form:

1. Events occurring during a run can be serialized to and deserialized from a "replay."
2. Loading a replay must **deterministically** reproduce the same state a live run reached — not "probably," provably, via a test.
3. **Everything** that happens during engine execution must be replayable — no event category is exempt.
4. The engine runs on a **fixed timestep** (discrete ticks), not wall-clock time.
5. `Event` must be **extendable** — application code can define its own event kinds and still benefit from the engine's built-in ones, through one uniform mechanism, with no special-casing between "built-in" and "custom."
6. State representation is an application concern; the engine core stays domain-agnostic.
   A small example lives in the actual game app.
7. No RNG/PRNG — deliberately out of scope, not even a reserved field for one later.
8. SOLID, small interfaces, tests written *alongside* each piece of behavior, not bolted on afterward.

Everything below is the story of turning that into working code.

## Starting point

The existing `event` library was closer to done than it looked.
`Event` was a bare `{ std::string name; }`.
`EventDispatcher` fanned a dispatched event out to `IEventSink`s and pushed it onto an `IEventQueue`.
`EventRecorder` was both an `IEventSink` and an `IEventHistory`, and already accumulated every dispatched event into a vector.
That's most of the shape of an event-sourcing log — it just had no concept of *when* (no tick), and `Engine::start()` didn't loop over time at all; it drained the queue once and returned.

So "add replay" turned out to mean: give the engine a real tick loop, teach the event pipeline to stamp everything with the tick it happened on, build a serialization format, and then prove the whole thing round-trips.

## Planning before code

Before writing anything, the work went through two rounds of planning (`PLAN.md`, revised once against explicit feedback) and a numbered checklist (`CHECKLIST.md`) with one short rationale note per item, written *as* each item landed rather than upfront.
That discipline is largely why the eventual bug (below) was caught by a test instead of shipped quietly — each checklist item had to end in a passing, specific test before it could be marked done, and "prove determinism" was one of those items, not an afterthought.

The planning docs and per-item notes lived under `docs/` during implementation.
They were scaffolding for *this* task, not permanent project documentation — which is why they're gone by the time you're reading this, and why this post exists instead: to keep the parts worth keeping.

## The core design

### Tick

`using Tick = std::uint64_t` landed in the `time` library — not a new library, just a type alias next to `IClock`.
It's the one thing every other piece depends on.

### Making `Event` extendable without a class hierarchy

The tempting design for "extendable events" is a polymorphic hierarchy — `Event` as a base class, subclasses per event kind.
It was considered and rejected.
It would need a name→decode factory to reconstruct concrete subtypes during deserialization, forces `unique_ptr` + cloning to keep value semantics, and none of that machinery is actually required to get the same result.

Instead, `Event` grew one field:

```cpp
struct Event
{
    std::string name{};
    std::string payload{};
    bool operator==(const Event &other) const = default;
};
```

`payload` is opaque bytes the engine never interprets.
The dispatch, queue, recording, and serialization machinery only ever needs to know "an event has a name and some bytes" — none of it has to change to support a new event kind, whether that's the engine's own built-in tick event or an application's `game.score_increment`.
That symmetry — built-in and custom events being *literally the same type*, flowing through *literally the same pipeline* — is what "no special-casing" ends up meaning in practice.

### Tick-stamping, without touching what already worked

Every dispatched event needs a tick attached before it can be recorded or replayed.
The obvious place to do that is `EventDispatcher` — but `EventDispatcher` already had passing tests asserting its exact behavior, and editing a tested, working class to bolt on a new concern is exactly the kind of change that quietly breaks something three call sites away.

So `TickedEventDispatcher` wraps it instead — a decorator that forwards to the real `IEventDispatcher` unchanged, then fans a `TimedEvent{tick, event}` out to a second set of sinks:

```cpp
void TickedEventDispatcher::dispatch(Event event)
{
    dispatcher.dispatch(event);
    TimedEvent timedEvent{.tick = currentTick, .event = std::move(event)};
    for (auto &sink : timedSinks) sink.get().handle(timedEvent);
}
```

`EventDispatcher` and its tests never changed.
Open/Closed, applied literally.

### Recording

`ITimedEventSink` / `ITimedEventHistory` / `ReplayRecorder` mirror the existing `IEventSink` / `IEventHistory` / `EventRecorder` almost line for line.
Registering `ReplayRecorder` as one of `TickedEventDispatcher`'s sinks gives a complete, ordered log of everything that happened in a run.

### A fixed-timestep engine

`IEngine::start()` used to drain the queue in one shot.
It now just logs a startup line.
The real work moved to a new method:

```cpp
virtual void step(antwika::time::Tick tick) = 0;
```

`Engine::step()` dispatches the engine's own built-in event (`events::kTick`, `"engine.tick"`) before draining that tick's queued events — the first "common event that comes with the engine," so application code gets per-tick hooks without inventing and dispatching its own tick signal.

### Serialization

A new `replay` library holds the persistence layer, split by responsibility: `IEventCodec` (one event ↔ bytes), `IReplayWriter` / `IReplayReader` (a whole sequence ↔ bytes, plus a small versioned header — magic bytes, format version, event count).
All I/O goes through `std::ostream&` / `std::istream&`, matching the existing `StreamAppender(std::ostream&)` pattern already in the logging library — tests get to use `std::stringstream` and never touch the filesystem.

Bad input — wrong magic bytes, an unsupported version, a truncated stream — throws one specific, catchable `ReplayFormatError`, not a vague `std::runtime_error` or (worse) silent misbehavior.

### Same code path, live or replayed

This is the part the determinism proof actually leans on.
`EngineLoop::run(totalTicks)` looks like this:

```cpp
for (Tick tick = 0; tick < totalTicks; ++tick)
{
    dispatcher.setTick(tick);
    for (auto &event : source.eventsFor(tick)) dispatcher.dispatch(std::move(event));
    engine.step(tick);
}
```

`source` is an `IReplaySource`.
In a live/scripted run it's built from a hand-authored `vector<TimedEvent>`; in a replay it's built from a deserialized one.
**The loop body is identical either way.**
That's not a minor implementation detail — it's the entire reason "replay reproduces the same state" is a provable property of the code instead of a hope.
If live mode and replay mode were two different code paths that happened to agree today, they'd be two code paths that could quietly stop agreeing tomorrow.

### State is the application's problem, not the engine's

The engine core has no idea what a "score" is, and it shouldn't.
State lives entirely in `apps/game`, as plain data plus a reducer:

```cpp
struct GameState
{
    std::uint64_t ticksProcessed{};
    std::uint64_t score{};
};

class GameStateReducer final : public ITimedEventSink
{
    void handle(const TimedEvent &event) override;
    // events::kTick            -> ticksProcessed++
    // "game.score_increment"   -> score += parse(event.payload)
};
```

The interesting part isn't the struct — it's that `GameStateReducer` is *just another* `ITimedEventSink`, registered the same way `ReplayRecorder` is.
"Recording history" and "folding events into state" turn out to be two implementations of the same idea, not two separate systems that both had to be invented.

## The bug the test caught

Everything above shipped commit by commit, each with its own passing tests.
Then came the one the whole project hinges on: run live, record, serialize, deserialize, replay into a fresh engine, assert the two runs match.

First attempt: serialize `ReplayRecorder`'s full history — the complete, correctly-recorded log of everything that happened, including the engine's own `engine.tick` events — and feed that back in as the replay's input.

The test failed.
Not flaky-failed — structurally, repeatably wrong.
The live run recorded 4 events over 3 ticks (one `engine.tick` per tick, plus one scripted event).
The replayed run produced **7**.

The cause, once found, was obvious in hindsight: `Engine::step()` *always* dispatches a fresh `engine.tick`, live or replayed, by design — that's what makes application code able to hook "every tick" uniformly.
But if the replay source *also* contains `engine.tick` entries (because it was built from the full recorded history, which correctly includes them), `EngineLoop` dispatches those too, on top of the ones `step()` generates itself.
Every tick that had a recorded `engine.tick` got it twice on replay.
The state didn't just differ — it diverged in exactly the way you'd predict once you see the double-dispatch.

The fix wasn't a patch, it was a distinction the code hadn't made yet: **a run's full observed history and a replay's required input are different things.**
`engine.tick` is a pure, deterministic function of the tick number — `Engine::step()` regenerates it identically every single time it's called.
It never needed to be *stored* as input, because it was never really an input; it's an output that happens to look like one if you're not paying attention to where it came from.
What a replay actually needs to persist is the external input — in this engine's current scope (no live/interactive input source yet), that's exactly the `vector<TimedEvent>` used to construct the original run's `IReplaySource` — which is a strictly smaller set than what `ReplayRecorder` observes.

The fix was one line different in the test: serialize the input script, not the recording.
Once that clicked, the assertion held — the *full* histories from both runs came out equal, including the independently regenerated `engine.tick` entries, because both runs really were following the identical deterministic path from the identical input.

This is the best kind of bug to find: caught by a test that existed *because* the checklist item said "determinism must be proven, not asserted by inspection," before it reached anything that could have shipped it quietly.
It also reshaped how record/replay got wired into the actual game app afterward — "recording a replay" doesn't need a capture step at all in this engine's current scope, since the caller already holds the input script it's about to run.
That's not a limitation so much as a natural consequence of not having interactive live input yet; it becomes real work again the day this engine gets one.

## Cleaning up afterward

Once the feature worked end to end — including `--record`/`--replay` flags on the actual `antwika_game` binary, smoke-tested by literally running a live session, replaying the saved file, and diffing the two logs byte for byte — the whole diff went through a second pass, purely for clarity: four independent reviews (reuse, simplification, efficiency, altitude) against the full `origin/main...HEAD` diff.

Two things were worth fixing:

- `BinaryPrimitives.cpp` had near-identical 32-bit and 64-bit read/write functions, differing only in width.
  Collapsed into one big-endian template, with the old names kept as thin wrappers so no call site changed.
- `IReplayWriter` / `IReplayReader` were interfaces with exactly one implementation each, never referenced polymorphically anywhere — production or test.
  Deleted; `BinaryReplayWriter` / `BinaryReplayReader` are concrete classes now.

A few other flagged items were deliberately left alone, and it's worth saying why rather than just silently ignoring them:

- `IEventCodec` also has one implementation — but `IFormatter` in the logging library is the exact same shape (one implementation, `PlainFormatter`) and is kept specifically so `Logger` can be unit-tested against `MockFormatter`, independent of real formatting logic.
  `IEventCodec` earns its keep the same way, even though nothing exercises that seam with a mock yet.
- `ITimedEventHistory` mirrors `IEventHistory`, which `EventRecorder` already implements in the pre-existing codebase for the same (currently-unexercised) reason.
  Removing one without the other would make `ReplayRecorder` stop looking like the class it was deliberately designed to resemble.
- `ReplaySource::eventsFor()` does a linear scan per tick — technically O(ticks × events).
  At today's scale (a handful of ticks, a couple of scripted events) it's not worth the complexity of an index.
  Worth revisiting the day replays get long enough for it to matter, not before.
- An automated review flagged `Engine::step()`'s queue-drain loop as "not participating in the new event pipeline."
  It doesn't need to — that loop's job (log whatever already got dispatched) is pre-existing behavior, unchanged in spirit; the events it logs were already fanned out to sinks earlier, at dispatch time.
  A useful reminder that automated review flags need a human to check them against what the code actually does before acting on them.

## Chasing branch coverage to 100%

CI's coverage report came back with four files short of full coverage — line coverage was actually fine everywhere except one closing brace; the real gap was in *branches* (90.8% overall).
Rather than guess from the percentages alone, the coverage build (`cmake --preset conan-coverage`) got rebuilt locally and inspected line by line via gcovr's JSON output, so each gap could be judged individually instead of chased blind.

They split cleanly into two categories:

**Real gaps, fixed with real tests:**
- `GameState`'s defaulted `operator==` had only ever been asserted `EXPECT_EQ` in existing tests — never compared as unequal, so the "these two fields differ" branch of the compiler-generated comparison had never fired.
  A small `GameStateTest.cpp`, matching the existing `EventTest.cpp`/`TimedEventTest.cpp` convention (equal, and inequality with only one field differing at a time), closed it.
- Neither `Game::run()` nor `Engine::step()` had a test for what happens when the *dispatcher itself* throws — every other "does this propagate an exception" test in the codebase (`EventDispatcherTest`, `EngineTest::Step_PropagatesExceptionWhenEventQueuePopFails`) already existed for other collaborators; these two were just missing their own instance of the same pattern.
- `BinaryReplayReader::read()` had a test for bad magic bytes and one for a truncated *trailing* field, but nothing for a stream too short to even contain the 4 magic bytes in the first place — a distinct failure mode from "wrong bytes."
- `BinaryPrimitives::readString` had never been tested with a string whose *length prefix* was intact but whose *content* was cut short — every existing truncation test happened to truncate a length field instead.

**Not real gaps — compiler-generated exception-safety artifacts:**
A couple of residual branches remained on lines like `dispatcher.dispatch(Event{.name = events::kTick});` even after the exception-propagation tests landed.
These are landing pads the compiler emits for the case where constructing the `Event` temporary itself throws (std::string allocation failure) — not reachable through any reasonable mock, and not our logic to test.
The codebase already had a precedent for exactly this situation: `Engine.cpp`'s `std::format(...)` lines were already marked `// GCOVR_EXCL_LINE` for the identical reason.
Applied the same marker to the residual lines, once real tests had covered everything that *could* be tested.
Same story for one bare closing brace in `ReplaySource::eventsFor()` — a return-value epilogue counter under `-O3` that no amount of new test scenarios moved, confirmed empirically before excluding it rather than assumed.

Result: 245/245 lines, 66/66 functions, 107/107 branches — every one of those either backed by a test that fails without the fix, or backed by a `GCOVR_EXCL_LINE` next to a comment explaining exactly why, following the convention the codebase already used rather than inventing a new one.

## Where it ended up

- 4 libraries touched (`time`, `event`, `engine`), one new one added (`replay`, 23 files).
- 80 tests, all added alongside the code they cover, none bolted on afterward.
- 17 commits, each independently green, each a single-line Conventional Commit.
- `antwika_game --record demo.replay` followed by `antwika_game --replay demo.replay` produces byte-identical log output and identical final state, verified manually as well as by an automated test that goes through the same `bootstrap()` entry point `main.cpp` uses.

If you're extending this system: new event kinds — engine-provided or application-defined — need nothing beyond a name and (optionally) a payload; state is a reducer implementing `ITimedEventSink`, wired in wherever your application assembles its dispatcher; and if you ever need to capture *live, interactive* input into a replayable script (rather than the current hand-authored one), that's the one piece of real, unbuilt work this project deferred — see the `engine.tick` story above for why it isn't as simple as "just record everything."
