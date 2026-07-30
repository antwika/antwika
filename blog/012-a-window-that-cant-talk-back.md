# A window that can't talk back

*Post 12*

`antwika::gfx` has been able to open a window and fill rectangles since the [SDL and raylib backends landed](../docs/gfx-plan.md), but the only thing using it was `apps/gfx_demo`, which draws three bars that never change.
Meanwhile `apps/life` had a board worth looking at and printed it as `#` and `.`.
This post is about connecting the two, and about the two decisions that turned out to be load-bearing — neither of which is "how do I draw a square".

## Drawing was the easy half

`BoardScene` takes a `Board`, a canvas size and an `IRenderer`, and emits one `clear`, one rectangle for the board's area, and one rectangle per living cell.
It is stateless, holds nothing, and is deterministic: the same board and canvas always produce the same calls in the same order.
That is the whole reason it exists as its own class rather than as code inside the render system — a picture that is a pure function of its inputs can be asserted against `MockRenderer` call by call, so `BoardSceneTest` checks the geometry instead of somebody checking the screen.

`RenderSystem` is then almost nothing: read the board out of the `World`, hand it to the scene, present.
It slots into the same `"observe"` phase `PrintSystem` already ran in, which is what makes it an *addition* rather than a rewrite — the ECS had a place for a per-tick read-only observer, and a renderer is one.

The one wrinkle is that it cannot hold a `Grid`.
`bootstrap()` is what creates the `Grid`, and `main` has to construct its observers before calling `bootstrap()`, so an observer can only be given the board's dimensions.
`PrintSystem` already solved this by counting cells off `world.view<Cell>()` and relying on `Grid` creating them row-major, so the read became a named function, `readBoardFromView`, that says out loud which convention it is leaning on and refuses to walk past the dimensions it was asked for.

## The renderer must not be allowed to close the window

Cell size is the smaller of `canvas.width / board.width` and `canvas.height / board.height`, and is never rounded up to a minimum of one.
That sounds like a nicety, and it is actually the thing keeping the arithmetic honest: taking the smaller quotient guarantees `cell * board.width <= canvas.width`, so centring with `(canvas.width - used.width) / 2` cannot underflow.
Clamp the cell size up on a canvas too small to hold the board, and that unsigned subtraction wraps to about four billion, which a cast to `std::int32_t` turns into a rectangle somewhere else entirely.
A canvas smaller than one pixel per cell draws nothing instead, and a test pins that.

The bigger trap was window lifetime.
`apps/gfx_demo` shows the obvious pattern: poll events, and on `CloseRequested`, call `window->close()`.
Doing the same thing here is wrong in a way no test in this repository would have caught.

`EngineLoop` finishes the tick that carries the stop event — deliberately, so that a live run and a replayed run agree about the terminal tick.
So if closing the window happens while that tick's events are being read, the observe phase still runs afterwards, and `RenderSystem` draws into a window whose framework resources have already been freed.
Under `NullBackend` nothing happens.
Under a `MockWindow` nothing happens.
Under SDL it is undefined behaviour, on a code path CI never exercises.

The fix is structural rather than defensive: the thing translating window events holds a `WindowId`, not an `IWindow &`, so it has nothing to call `close()` on.
Window lifetime stays with `main`, which owns the `unique_ptr` for the whole run.
And that in turn means `RenderSystem` needs no `isOpen()` guard — which is good, because a guard that can never be false is a branch a 100% coverage gate would demand a test for and no honest test could produce.

## Closing a window is input, so it goes where input goes

The [gfx plan](../docs/gfx-plan.md) asked for this up front: `antwika::gfx` must not depend on `antwika::event`, so turning a `gfx::WindowEvent` into an `antwika::event::Event` is the application's job.
`WindowInputSource` is that adapter, and it is a decorator over `IReplaySource`:

```cpp
std::vector<Event> WindowInputSource::eventsFor(antwika::time::Tick tick)
{
    auto events = inner.eventsFor(tick);

    bool closeRequested = false;
    while (const auto event = backend.pollEvent())
    {
        if (event->window != window) { continue; }
        if (std::holds_alternative<CloseRequested>(event->payload))
        {
            closeRequested = true;
        }
    }

    if (closeRequested)
    {
        events.push_back(Event{.name = engine::events::kStop});
    }

    return events;
}
```

Fourteen lines, and they buy something disproportionate.
Because a window close arrives as an ordinary event on an ordinary tick, `TickEventRecorder` records it like anything else, `saveReplayFile` writes it out, and a `--record` run that ends when somebody closes the window produces a replay that stops at exactly that tick.
Nothing in the engine, the loop or the recorder learned what a window is.

There is a test for that, and it is the check the plan named as Phase 6's exit criterion: record a run under a mocked windowed backend, close the window part way through, then replay the saved file under the real `NullBackend` and assert the boards match.
It also asserts the recording's last event is `engine.stop` at the tick the window was closed on, because two runs that both did nothing would agree for the wrong reason.

The one thing that test taught me: a recording cannot be fed straight back in.
`TickEventRecorder` records `engine.tick` too, and `engine.tick` is what makes `BoardSink` commit the world and run the scheduler.
Replay a recording with the ticks still in it and every tick advances two generations.
`saveReplayFile` has always filtered them — [that is what "never really input" meant](009-json-wins-tickevent-and-three-mains-that-stopped-repeating-themselves.md) — so the test round-trips through the real save and load rather than shortcutting, which is also the only way it exercises what `main` actually relies on.

## Where the honesty runs out

`WindowInputSource` sits in front of a `--replay` run as well as a live one.
So closing the window half way through a replay injects a stop the file does not contain, and the run ends early.
That is the right behaviour — a window somebody closed should close — but it means the guarantee is one-directional: a replay reproduces a recorded run exactly *if it is left to finish*.
The plan's phrasing ("record under a real backend, replay under `NullBackend`, assert the final state is identical") happens to describe precisely the case that is airtight, which was luck rather than foresight.

## The demo had to become worth watching

None of the above is visible on a 5×5 board that stops after three ticks.
The board became 32×32 and the seed became a glider, which crosses it diagonally in about 116 generations before it runs into the corner and settles into a block.
`engine.stop` sits at tick 130, a little past that, so the final state holds for a moment before the window goes away.

Then the tick loop turned out to be far too fast to watch — 130 generations land in under a millisecond.
Waiting is not something the loop can do: `antwika::replay` exists to be independent of wall-clock time, and `WindowInputSource` runs *before* the frame is drawn, so a wait there would pace the gap between reading input and drawing rather than between frames.
So `TickPacer` is an `ISystem` that ignores both of its arguments and sleeps, registered after the render system, which makes the order present-then-wait.
An observer that observes nothing is a slightly odd object, but it is in the right place, and it is the only place waiting can happen without a library learning about clocks.

The last decision was what to do with `PrintSystem`.
"Instead of the console" was the goal, and deleting the ASCII output is nevertheless wrong, because `null` is the *default* backend: a default build with no printer produces a program that opens no window and says nothing, which reads as broken to anyone who just cloned the repository.
So `main` asks the backend its name and registers the printer when it draws nothing — and skips the pacing in the same breath, since a build with nothing to watch has nothing to wait for.
The gfx plan said "replace or complement".
The default backend is the argument for complement.
