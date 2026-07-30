# Plan: two categories of input, and a replay that stops growing

A plan for cutting what a `--record` run writes, by separating the input a run's outcome depends on from the input that merely says where the pointer currently is, and dropping the second category before it ever reaches the recorder.

**Status: built.**
Every phase below has landed, including the optional fifth.
Where the plan turned out to be wrong while it was being built, the text says so under [What changed while building it](#what-changed-while-building-it) rather than being quietly corrected, since the reasoning is the point of keeping this document.

## Context

A recorded session grows at the rate the window system reports pointer motion, which is not the rate the application ticks at.
`apps/game` ticks every 40 ms, so it reads the pointer 25 times a second, while SDL will happily report several hundred movements a second during a drag.
Every one of those becomes an `input.pointer_move` in the file.

Pretty-printed, one of them costs about 140 bytes:

```json
    {
      "tick": 1234,
      "event": {
        "name": "input.pointer_move",
        "payload": "{\"x\":512,\"y\":128}"
      }
    },
```

The library already has the decorator meant to stop this.
`antwika::input::CoalescingPointerSource` keeps only the last of each run of movements inside a tick, and its header states the invariant that makes it safe: it sits upstream of `TickEventRecorder`, so what a recording holds is exactly what the run consumed.

Two things are wrong with how that decorator is deployed today, and both are worth fixing before any new mechanism is designed.

### `apps/game` coalesces the wrong stream

[`src/apps/game/src/main.cpp:176`](../src/apps/game/src/main.cpp#L176) builds the chain in this order:

```cpp
CoalescingPointerSource coalesced(fileSource);
LiveInputSource liveSource(coalesced, *inputBackend, codec);
IReplaySource &inner = live ? liveSource : fileSource;
```

`LiveInputSource::eventsFor()` calls `inner.eventsFor(tick)` and then appends one event per edge it polls off the backend.
The coalescer is that inner source, so it thins the *scripted* events and never sees a single live one.
Live pointer motion reaches the recorder untouched, at whatever rate the framework reports it.

The `--replay` branch is worse: `inner` is `fileSource` directly, so the coalescer is not in that path at all.
As currently wired, `CoalescingPointerSource` does nothing in either mode.

### `apps/life` does not coalesce at all

[`src/apps/life/src/main.cpp:152`](../src/apps/life/src/main.cpp#L152) never mentions the decorator.
That is not an oversight to fix by adding it: coalescing is lossy for a *path*, and `life::PointerToggleSink` toggles every cell a drag crosses, so collapsing a run of movements inside a tick would skip cells at speed.
`life` needs the second mechanism in this plan and must not be given the first.

### What is left after that is still the real problem

Fix the wiring and `game` records at most one movement per tick per run, which is 25 events a second, or about 3.5 KB/s pretty-printed.
That is 200 KB a minute for a session where the pointer is simply crossing the window, and the overwhelming majority of those movements changed nothing at all: `GridSink` pans only while the middle button is held, and `PointerToggleSink` toggles only while the left button is held.

That is the observation this plan is built on.

## The two categories

- **Load-bearing input.**
  The run's outcome depends on it.
  A press, a release, a scroll, a key, a window close, and a movement that arrives while a button is held.
  These must be in the file, or the replay reaches a different state.
- **Ambient input.**
  It only updates where the application believes the pointer to be, and nothing reads that belief before the next load-bearing event supersedes it.
  A movement while no button is held is the only member of this category today.

The distinction the user of a replay cares about is exactly the one `REQUIREMENTS.md:18` already draws for `engine.tick`: a replay persists external input and nothing that can be regenerated.
Ambient motion is not regenerable in that sense — it is genuinely external — but it is *inconsequential*, which is a second, weaker reason to leave it out.

## Decision: the category is a property of the moment, not of the name

The tempting implementation is a list of event names, next to `kSelfGeneratedEventNames` in each app's `main.cpp`, filtered out by `saveReplayFile()`.
It cannot work, for two independent reasons.

**`input.pointer_move` belongs to both categories, minutes apart in the same session.**
A movement while the middle button is held pans `game`'s camera, which is simulation state that clicks are resolved against.
The same event name, with the button up, changes nothing.
Whether an event is load-bearing depends on the folded input state at the moment it arrives, so anything deciding it has to be folding that state.

**Filtering at save time breaks the invariant the whole design rests on.**
`CoalescingPointerSource`'s header says it plainly, and `REQUIREMENTS.md:14` says it as a requirement: everything that happens during engine execution must be replayable, no event category exempt.
A filter downstream of `TickEventRecorder` produces a file that disagrees with the run that wrote it, and the disagreement is silent — the replay runs, reaches a different state, and nothing says so.
A filter upstream of the recorder cannot do that, because the events it drops never happen during engine execution at all.
Requirement 14 keeps holding, unchanged, and no new exemption is introduced.

So: **the split is a stateful decision, made in an `IReplaySource` decorator, upstream of the recorder** — the same place, and for the same reason, as the coalescing that is already there.

## Decision: drop, but latch

Dropping a movement outright is not safe, because folded position outlives the event that set it.
`GridSink` anchors a zoom at `state.mouse().position()`, and `input.pointer_scroll` carries no position of its own.
Move the pointer with no button held, then scroll: if the movements were dropped, the zoom anchors wherever the pointer was last *recorded* to be, which may be minutes and half a window away.

The gate therefore keeps the last movement it dropped, and releases it into the stream immediately before the first event it does *not* drop:

| Arriving | Held | Emitted |
| --- | --- | --- |
| movement | none | nothing; it replaces whatever was latched |
| movement | any button | the movement |
| press, release, scroll, key, stop | any | the latched movement, if any, then the event |

Everything that reads position reads it at one of the moments in the third row, and at every one of those moments the position is exactly what it would have been without the gate.
The re-emitted movement carries a later tick than the one it physically arrived on, which is invisible downstream and, because this happens upstream of the recorder, is what the file says too.

Releasing before a press is what makes the gate safe for `life`: the latched movement arrives while `drag.inProgress()` is still false, so it toggles nothing, and the press that follows starts the drag at its own position.

## Phases

### Phase 0: measure

Record a minute of ordinary play before changing anything, so every later phase can be judged against a number rather than an argument:

```sh
build-sdl3/bin/antwika_game --record /tmp/before.replay
jq -r '.events[].event.name' /tmp/before.replay | sort | uniq -c | sort -rn
wc -c /tmp/before.replay
```

Keep the file.
It is also the input for Phase 4's idempotence test.

### Phase 1: put the coalescer where it can see live input

No new code, only ordering, in [`src/apps/game/src/main.cpp`](../src/apps/game/src/main.cpp):

```cpp
LiveInputSource liveSource(fileSource, *inputBackend, codec);
IReplaySource &polled =
    live ? static_cast<IReplaySource &>(liveSource)
         : static_cast<IReplaySource &>(fileSource);

// Outside the live source, so it sees what the device reported.
// In both branches, so a replay is thinned exactly as the run was.
CoalescingPointerSource coalesced(polled);
StopOnKeySource quitting(coalesced, codec, kQuitKey);
```

Putting the coalescer in the `--replay` branch too is deliberate: the two branches must run the same pipeline, or a hand-authored file with several movements in a tick replays differently from the live run that would have produced it.
It is safe to re-apply to an already-coalesced file, since coalescing is idempotent.

Add a test that fails on the old wiring: drive a `FakeInputBackend` that reports several movements between two ticks, run with a recorder attached, and assert the recording holds one movement per tick.
`src/libs/input/tests/InputDeterminismTest.cpp` already has the harness for this.

Expected effect: from "whatever the framework reported" down to 25 events a second while the pointer moves.

### Phase 2: `antwika::input::IdleMotionSource`

The gate, as its own decorator beside `CoalescingPointerSource` — a separate class rather than an option on that one, because the two answer different questions and `life` needs exactly one of them.

`src/libs/input/include/antwika/input/IdleMotionSource.hpp`:

```cpp
/**
 * @brief Drops pointer movement that arrives while no button is held,
 * latching the last of it until something reads a position.
 *
 * A movement with a button down is doing something -- panning a camera,
 * drawing on a board -- and is passed through untouched. A movement with
 * every button up only updates where the application believes the pointer
 * to be, and nothing reads that belief until the next press, release,
 * scroll or key arrives. So the last such movement is held back and
 * released immediately before that event, which is the only moment its
 * position could be read. Everything downstream sees the position it
 * would have seen anyway; what it does not see is the several hundred
 * intermediate positions it would have thrown away.
 *
 * Safe for determinism for the reason CoalescingPointerSource is: it
 * sits upstream of TickEventRecorder, so a recording holds exactly what
 * the run consumed, and the run and its replay see the same stream.
 *
 * **Not for an application that draws anything following a free-moving
 * pointer.** A hover highlight, a rubber band or a custom cursor updates
 * only when a button, a wheel or a key does, because between those the
 * movements are not there to draw from.
 */
class IdleMotionSource final : public IReplaySource
{
public:
    /**
     * @brief Construct the decorator over what it wraps.
     * @param inner The source whose events pass through; must outlive
     * this object.
     * @param codec Used to recognise movement and fold button state.
     */
    IdleMotionSource(IReplaySource &inner, const IInputEventCodec &codec);

    // ... deleted copy and move, as every source here has ...

    /**
     * @brief Get a tick's events, with idle movement held back.
     * @param tick The tick to fetch events for.
     * @return The wrapped source's events, less any movement that
     * arrived with no button held, plus the last such movement ahead of
     * the first event that is not one.
     */
    [[nodiscard]] std::vector<Event> eventsFor(
        antwika::time::Tick tick) override;

private:
    IReplaySource &inner;
    const IInputEventCodec &codec;

    // Folded from what passes through, never read from a device.
    // Only isDown() is asked of it, so dropping movement cannot skew it.
    InputState state;

    // The last movement dropped, still unread by anything.
    std::optional<Event> latched;
};
```

The implementation is one loop over `inner.eventsFor(tick)`:

- `codec.decode()` the event; a `std::nullopt` (an app's own event, `engine.stop`) is not input, so release the latch ahead of it and pass it on.
- A `PointerMoved` with no button held: fold it, move it into `latched`, replacing whatever was there, and emit nothing.
- Anything else: emit the latch if there is one, clear it, fold, emit the event.

`Mouse` answers `isDown(button)` but has no "is anything down", so it gains one:

```cpp
/**
 * @brief Get whether any button at all is currently held.
 * @return True if at least one button is down.
 */
[[nodiscard]] bool anyDown() const noexcept;
```

One line over the `std::bitset` already there, beside `isDown()`, and covered by the gate's tests plus a pair of its own in `MouseTest.cpp`.

The gate holds an `InputState` above the recorder, which `GridSink`'s comment warns against for *its* state — the difference is that this one only ever answers "is a button down", an answer no dropping can change, and its decisions are baked into the recorded stream rather than derived from it.

Tests, in `src/libs/input/tests/IdleMotionSourceTest.cpp`:

- an idle movement is dropped
- a movement with a button held passes through
- the last of several idle movements is what gets released, and only one is released
- the latch is released before a press, a release, a scroll, a key and a non-input event
- the latch survives across ticks and is released on a later one
- a press, then movement, then a release, then movement: the first movement passes, the second is latched
- nothing is emitted twice, and a tick with no events stays empty
- running an already-gated stream through a second gate changes nothing

### Phase 3: attach it

`game`, outside the coalescer so the gate sees at most one movement per run:

```cpp
CoalescingPointerSource coalesced(polled);
IdleMotionSource gated(coalesced, codec);
StopOnKeySource quitting(gated, codec, kQuitKey);
```

`life`, with no coalescer, for the reason in [Context](#appslife-does-not-coalesce-at-all):

```cpp
IdleMotionSource gated(*seeded, codec);
WindowInputSource source(gated, *backend, window->id());
```

`poker` and `task_worker` read no pointer and are untouched.

Expected effect: movement is recorded only while a button is held, which for a build-and-look-around session is a small fraction of the run.

### Phase 4: prove it end to end

Extend the existing determinism tests rather than writing new harnesses:

- `src/libs/input/tests/InputDeterminismTest.cpp`: a scripted device stream containing plenty of idle movement, run live with the full pipeline and a recorder; assert the recording is materially shorter than the raw stream, then replay it through the same pipeline and assert the folded end state is identical.
- `src/apps/game/tests/ReplayDeterminismTest.cpp`: the same, asserting on `GameSummary` — paths, walkers, and the camera's pan and zoom, since the camera is what a dropped movement would most plausibly break.
- Re-record Phase 0's session and compare: `jq '.events | length'` on both files, in the commit message if not in a test.

### Phase 5 (optional): stop pretty-printing recordings

`ReplayWriter::write()` calls `dump(2)`.
Roughly 40% of a recording is indentation and newlines that only a human reading a checked-in demo file benefits from.

Give `ReplayWriter` a constructor taking `enum class Layout { Pretty, Compact }`, defaulting to `Pretty`, and an optional argument on `saveReplayFile()` to match, so the demo replays under `src/apps/*/replays/` stay diffable while `--record` output does not.
`ReplayReader` needs no change; the schema does not either.

Worth doing after Phases 1-3, not before: it is a constant factor on a number those phases make much smaller.

## What changed while building it

**`Mouse` gained `anyDown()` after all.**
The plan already expected it, and it is one line over the `std::bitset` that was there.

**The gate calls `beginTick()` on its own `InputState`.**
Nothing reads the per-tick sums it clears -- only `anyDown()` is asked -- but `Mouse::delta()` and `Mouse::scroll()` accumulate whether or not anybody reads them, and a session long enough to motivate this work is long enough to overflow a `std::int32_t` that nothing ever empties.

**`Layout` is nested inside `ReplayWriter`, not free in the namespace.**
`ReplayWriter::Layout::Compact` says what it configures at the call site; a bare `Layout` in `antwika::replay` would not.

**Compact saves about a third, not the "roughly 40%" Phase 5 guessed.**
Measured over a recording of twenty pointer movements: 2,796 bytes pretty against 1,720 compact, so 38% goes.
The test asserts the third rather than the guess.

**The counts in Phase 4 are now assertions.**
A `game` session of fifteen wandering movements, a middle-drag and a zoom records 21 input events ungated and 8 gated.
The `life` equivalent goes from 9 movements to 6, since four of its nine are mid-drag and have to survive.
Both numbers are in tests rather than in this document, so they cannot quietly rot.

## What this costs

**No hover.**
An application attaching `IdleMotionSource` cannot draw a cell highlight that follows the pointer, because the movements that would drive it are not in the tick stream between clicks.
Neither `game` nor `life` draws one today.
When one wants to, the honest options are to drop the gate for that app and pay the file size, or to move the highlight to something that updates on click — not to read the device from the renderer, which `REQUIREMENTS.md:40` and `:64` both forbid.

**The recorded path is not the physical path.**
Already true after coalescing, and this widens the gap.
A replay reproduces the state a run reached, not the trajectory the hand took.
Any future gesture recogniser, freehand drawing tool or velocity-based fling must be built without either decorator attached.

**One more thing to get right per app.**
The gate is opt-in per `main.cpp`, so a new pointer-reading app gets no benefit until someone attaches it, and gets a subtly wrong hover if they attach it without reading this.
That is the cost of the classification being a property of the moment: it cannot be decided centrally.

## Deferred deliberately

- **Dropping key repeats.**
  A held key reports at the OS repeat rate, roughly 30 a second, and `StopOnKeySource` already ignores repeats.
  Nothing in the repository acts on one, so this would be speculative until something does; the same latch-free variant of the gate would do it.
- **A binary replay format.**
  JSON is a deliberate choice (`blog/009`), and the sizes this plan reaches do not justify revisiting it.
- **Storing the payload as a nested object rather than an escaped string.**
  `"{\"x\":512,\"y\":128}"` pays for every quote twice.
  It is a replay format version bump for perhaps 15%, and should wait until the format changes for a reason of its own.
- **A per-tick index for `ReplaySource::eventsFor()`.**
  Already deferred by `REQUIREMENTS.md:93`, and this plan makes long replays likelier, so it may become due — but on its own evidence, not this one's.
- **Deciding the category from what a sink actually consumed.**
  A sink reporting "I did nothing with this" would classify precisely instead of by rule, and would need no per-app wiring.
  It also means feedback from downstream of the recorder back into what the recorder keeps, which is the one direction this design does not allow.

## Requirements to add

To `REQUIREMENTS.md`, under Should have:

- Input that cannot affect the state a run reaches should be dropped before it reaches the replay recorder, never filtered out of a recording afterwards, so a replay file always holds exactly what its run consumed.
- Whether a pointer movement can affect state should be decided from the folded button state at the moment it arrives, not from its event name, since the same name is load-bearing during a drag and inconsequential outside one.

To Won't have (this scope):

- An application attaching the idle-motion gate won't draw anything that follows a free-moving pointer, since the movements between clicks are deliberately not in the tick stream.
