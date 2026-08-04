# apps/life

`src/apps/life/` — Conway's Game of Life in an ECS world.

## What it demonstrates

Application state held in an [`ecs`](../libraries/ecs.md) `World` rather than a plain struct, and live pointer input turned into state changes inside the tick path.

## Running it

```sh
build/bin/antwika_life/antwika_life
build/bin/antwika_life/antwika_life --record demo.replay
build/bin/antwika_life/antwika_life --replay src/apps/life/replays/demo.jsonl
```

It opens a window, draws the board each tick, and takes mouse input; drag over cells to toggle them.
The run is uncapped and it has no end of its own: only closing the window, reported through `WindowInputSource`, or a replay dispatching `engine.stop` ends it.
A headless build reports neither, so `Ctrl+C` ends one — and a `--record` run keeps what it got to, since a recording is appended a line at a time rather than written after the run.

## Libraries it composes

[`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`engine`](../libraries/engine.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`time`](../libraries/time.md), plus the selected backends.

## How it is put together

Each cell is an entity carrying a `Cell` component.
`LifeSystem` advances every cell one generation per tick through the double-buffered `World` and `SystemScheduler`.
`Board`, `Grid` and `BoardSink` handle the scripted `life.toggle_cell` event; `PointerToggleSink` handles the mouse.
`BoardScene`, `RenderSystem` and `PrintSystem` are the write-only output side.
Both the windowed and the headless run are paced through `TickPacer`, since a run that never ends would otherwise go flat out.

## Non-obvious decisions

**A drag is recorded; the toggles it caused are not.**
`input::LiveInputSource` puts each edge into the tick stream and `PointerToggleSink` decodes the `input.pointer_*` events and toggles the cell under the pointer, so a `--record` run persists the click and regenerates the toggle.

**Where a cell is drawn and which cell a click lands in are one function.**
`layoutFor()` and `cellAt()` in `BoardLayout.hpp` are shared by `BoardScene` and `PointerToggleSink`, so the two cannot drift.
That mapping is against the *configured* window size rather than the size a window reports, and the window is not resizable, which is what keeps a recorded session landing on the same cells under a different backend.
`RenderSystem` reads the same `configuredSize()` as `PointerToggleSink` does, since one function taking two different sizes is two functions: a window manager that hands back a size of its own choosing -- which a tiling one routinely does -- would otherwise draw the board at one scale and hit-test it at another, and every click would land on the wrong cell.

**The sink keeps its own note of what the current tick staged.**
`PointerToggleSink` toggles a cell at most once per drag, and the `World` hands out the *committed* board, which would otherwise let two drags over one cell in a tick collapse into a single toggle.

**Holding the button pauses the generations, and only the generations.**
The sink reports the drag through a shared `DragState`, and `DragPausedSystem` wraps `LifeSystem` and stages nothing while a drag is under way.
The tick, the commit and every observer still run, so the cells being painted appear immediately — which is the whole point of pausing.
A press that lands off the board still pauses, since what pauses is holding the button rather than hitting a cell.
Which ticks were paused follows from the recorded presses and releases, so a replay pauses on exactly the same ones.

**Only the gate, not the coalescer.**
This app attaches `input::IdleMotionSource` but not `CoalescingPointerSource`, because a drag toggles every cell it crosses and thinning a run of movements inside a tick would skip some.

See [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](../../blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md) and [`blog/004-a-game-of-life-demo-and-a-queue-nobody-was-reading.md`](../../blog/004-a-game-of-life-demo-and-a-queue-nobody-was-reading.md).

## The debug console

The grave key drops `antwika::console`'s sheet over the board, with the shipped constants: Grave to toggle, Enter to execute, the Swedish typing board.
`InputFold` runs first in the sink list and `ConsoleSink` immediately after, and `PointerToggleSink` -- the one sink here that reads a key or a pixel -- is wrapped in a `ConsoleGatedSink`, so a press under the sheet toggles no cell while one below it still does.
`BoardSink` and `StopSignal` are not wrapped, since neither reads input, and the generations do not pause while the console is open: the sheet is a surface over the run, not a hold on it.
`RenderSystem` paints the console's picture after the board and before `present()`, off the same `ConsolePicture` the sink describes into.

`dump_state` writes the whole state to `dump_state.json` and `load_state` reads it back: the `Board`, whether a drag was under way, which cells that drag had already toggled and where it last was -- so coming back to a dump means coming back to the instant it was taken, mid-drag and all.
The file is `console::SnapshotFormat`'s shared envelope under this application's own magic, carrying the console's history beside the state, and `LifeSnapshotStore` owns what the state *is* while `console::SnapshotCommands` owns the policy.
A load stages every cell into the `World`, where it lands at the next commit exactly as a toggle does, and is refused with a history line -- never performed -- while recording or replaying, since the file's contents are nothing a recording carries.
`bootstrap()` returns a `LifeSummary`, the board plus the console's history, so a replay-determinism comparison covers what the console said as well as what the cells became.

## The config file

`config.json` beside the executable is read once at startup through [`antwika::config`](../libraries/config.md), and holds the one number this application is willing to move: `tickIntervalMs`, how long a tick takes on the wall clock.
Pacing is safe to move because a pacer changes how long a tick takes and never what it computes; the board's size stays in source, since the cell a click means is worked out from it.
A missing file is the shipped application, and a broken one is refused at startup rather than repaired.
