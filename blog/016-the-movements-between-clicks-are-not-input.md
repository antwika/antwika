# The movements between clicks are not input

*Post 16*

`apps/game` ticks every 40 ms, so it reads the pointer twenty-five times a second.
SDL will happily report several hundred pointer movements a second while you drag.
Every one of those was becoming an `input.pointer_move` in the `--record` file, and pretty-printed one of them costs about 140 bytes:

```json
    {
      "tick": 1234,
      "event": {
        "name": "input.pointer_move",
        "payload": "{\"x\":512,\"y\":128}"
      }
    },
```

A minute of somebody moving the mouse around without clicking anything is hundreds of kilobytes of a file that describes nothing happening.

This post is about cutting that, and about the one place the cut is allowed to be made.

## The decorator that was already there and did nothing

`CoalescingPointerSource` — keep only the last of a run of movements inside a tick — had existed since the input library landed.
Its header states the invariant that makes it safe: it sits upstream of `TickEventRecorder`, so a recording holds exactly what the run consumed.

It was wired into `apps/game` like this:

```cpp
CoalescingPointerSource coalesced(fileSource);
LiveInputSource liveSource(coalesced, *inputBackend, codec);
IReplaySource &inner = live ? liveSource : fileSource;
```

`LiveInputSource::eventsFor()` calls its inner source and *then* appends one event per edge it polls off the backend.
The coalescer is that inner source, so it thinned the scripted events and never saw a single live one.
And the `--replay` branch bypasses `liveSource` entirely, so the coalescer was not in that path at all.

As wired, it did nothing in either mode.
That is the most instructive kind of bug in a decorator chain: everything compiles, every test passes, and the order of two constructor arguments is the whole behaviour.
The chain is built in one place now, `input::InputPipeline`, from a struct of options rather than by hand in each `main.cpp`:

```cpp
InputPipeline input(
    fileSource,
    *inputBackend,
    codec,
    {.readsDevice = !recorded.options.replayPath.has_value(),
     .coalescePointerMotion = true,
     .thinIdleMotion = true,
     .stopOnKey = kQuitKey});
```

Four booleans, and the nesting order that makes them mean what they say is written once.

## Fixing the wiring is not fixing the problem

Coalesced properly, `game` records at most one movement per tick — twenty-five a second, about 3.5 KB/s, still 200 KB a minute for a pointer that is merely crossing the window.

And the overwhelming majority of those movements changed nothing whatsoever.
`GridSink` pans only while the middle button is held.
`life::PointerToggleSink` toggles only while the left button is held.
Everything else the movements do is update where the application *believes* the pointer to be, and nothing reads that belief before the next movement supersedes it.

That is the observation the rest of this is built on.
There are two categories:

- **Load-bearing input**, which the run's outcome depends on: a press, a release, a scroll, a key, a window close, and a movement that arrives while a button is held.
- **Ambient input**, which only updates a belief nothing reads: a movement while no button is held.

## The category is a property of the moment, not of the name

The tempting implementation is a list of event names filtered out by `saveReplayFile()`, next to the self-generated names it already drops.
It cannot work, for two independent reasons.

**`input.pointer_move` belongs to both categories, minutes apart in the same session.**
A movement with the middle button held pans the camera, which is simulation state that every later click is resolved against.
The same event name with the button up changes nothing.
Whether an event is load-bearing depends on the folded input state at the moment it arrives — so whatever decides it has to be folding that state, and a name is not enough to go on.

**Filtering at save time breaks the invariant the whole system rests on.**
Everything that happens during engine execution must be replayable, with no exempt category.
A filter downstream of `TickEventRecorder` produces a file that disagrees with the run that wrote it, and the disagreement is silent: the replay runs, reaches a different state, and nothing says so.
A filter *upstream* of the recorder cannot do that, because the events it drops never happen during engine execution at all.

Doing it in a backend is out for the same reason from the other side — it would hide the reduction behind the seam, where no application can see it and no replay can account for it.

So the split is a stateful decision, made in an `IReplaySource` decorator, upstream of the recorder.
The same place, and for the same reason, as the coalescing already there.

## Drop, but latch

Dropping an ambient movement outright is not safe, because folded position outlives the event that set it.

`GridSink` anchors a zoom at the pointer's folded position, and `input.pointer_scroll` carries no position of its own.
Move the pointer with no button held, then scroll: with the movements dropped, the zoom anchors wherever the pointer was last *recorded* to be — possibly half a window and a minute away.

So `IdleMotionSource` keeps the last movement it dropped and releases it immediately ahead of the first event it does not drop:

- A movement with no button held emits nothing, and replaces whatever was latched.
- A movement with any button held is emitted as it arrived.
- A press, release, scroll, key or stop emits the latched movement first, if there is one, and then itself.

The code is the table:

```cpp
if (moved != nullptr && idle)
{
    // Whatever was latched was superseded unread.
    latched = std::move(event);
    continue;
}

// This is the first moment anything could read a position.
if (latched.has_value())
{
    kept.push_back(std::move(*latched));
    latched.reset();
}
```

Everything that reads a position reads it at one of those moments, and at every one of them the position is exactly what it would have been without the gate.
The released movement carries a later tick than the one it physically arrived on, which is invisible downstream — and because this is upstream of the recorder, it is what the file says too.

Releasing *before* a press is also what makes the gate safe for `apps/life`: the latched movement arrives while the drag is still not in progress, so it toggles nothing, and the press that follows starts the drag at its own position.

One line in there looks like dead code and is not:

```cpp
// Only anyDown() is read below, which this does not clear.
// It is called so the per-tick sums cannot run away unread.
state.beginTick();
```

The gate only ever asks its `InputState` whether any button is down.
But `Mouse::delta()` and `Mouse::scroll()` accumulate whether anybody reads them, and a session long enough to motivate this work is long enough to overflow a counter nothing ever empties.

## Which decorator an app may attach is an app-level question

`game` takes both.
`life` takes only the gate.

Coalescing is lossy for a *path*, and `PointerToggleSink` toggles every cell a drag crosses.
Collapsing a run of movements inside a tick would skip cells whenever the hand moves quickly, which is a bug you would blame on the mouse.
The gate never touches a movement made with a button down, so it is safe for a drag by construction.

Measured, on sessions the tests now pin: a `game` session of fifteen wandering movements, a middle-drag and a zoom records 21 input events ungated and 8 gated.
The `life` equivalent goes from 9 movements to 6, because four of its nine are mid-drag and have to survive.
Both numbers are assertions in tests rather than sentences in a document, so they cannot quietly rot.

## What it costs, said plainly

**No hover.**
An application attaching the gate cannot draw anything that follows a free-moving pointer — a cell highlight, a rubber band, a custom cursor — because the movements that would drive it are not in the tick stream between clicks.

This is why `apps/game`'s toolbar buttons light up on the press rather than on approach, and it is worth being precise about whose decision that is.
It is not something `antwika::ui` decides: `resolve()` hit-tests whatever pointer it is handed, every frame.
It is this app's pipeline choice, made in `main.cpp`.
Clicking is unaffected either way, since the gate releases the latched movement ahead of the press, and a press carries its own position.

**Postscript: this cost was later removed, and the framing above was wrong.**
For a while the trade read as a dilemma — keep the gate and lose hover, or drop it and pay the recording size the gate exists to save.
That was never the only shape available, and `input::PointerHintChannel` is the answer we should have reached for first.
A free-moving position is published once per tick into a plain value cell that no dispatcher carries and no recorder can see, so an application draws a hover from it while the recording stays exactly as thin as the gate made it.
The two mechanisms turn out to be complementary rather than opposed: the gate thins what is written and publishes nothing, the channel publishes and thins nothing.

The reason it looked like a dilemma is worth keeping, because it is the more general mistake.
Everything in this post is about the event stream, so the search for an answer stayed inside the event stream, where the only lever is which events survive.
Once the question is asked as "what may a renderer read?" rather than "which events may I drop?", the answer stops being a compromise.
The safety condition is the whole of it: an unrecorded position may feed render-side state only, and never anything a replay has to reproduce.
Put it in an event and that condition is a rule every sink must remember; put it in a value cell no sink is handed, and the rule is structural.

**The recorded path is not the physical path.**
Coalescing already made that true; the gate widens it.
A replay reproduces the state a run reached, not the trajectory the hand took.
Any future gesture recogniser, freehand drawing tool or velocity-based fling has to be built with neither decorator attached.

**One more thing to get right per app.**
The gate is opt-in per `main.cpp`, so a new pointer-reading app gets no benefit until somebody attaches it — and gets a subtly wrong hover if they attach it without reading this.
That is the unavoidable cost of the classification being a property of the moment: it cannot be decided centrally.

## The rename that was rejected

There was a proposal to rename `IdleMotionSource` to something like `DiscardsHoverMotion`, so that the caveat is in the name.

It was reviewed and turned down.
`IdleMotionSource` describes what the decorator does to the stream, which is the level an `IReplaySource` is named at, and it is true of every application.
"Discards hover" describes a consequence for an application that draws a hover — a property of *that* application, and one neither `apps/life` nor a headless run has at all.

The caveat belongs where an application chooses the behaviour, so it is written on `InputPipelineOptions::thinIdleMotion`: the field a call site actually sets, and the last place anybody looks before turning it on.
