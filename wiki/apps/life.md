# apps/life

`src/apps/life/` — Conway's Game of Life in an ECS world.

## What it demonstrates

Application state held in an [`ecs`](../libraries/ecs.md) `World` rather than a plain struct, and live pointer input turned into state changes inside the tick path.

## Running it

```sh
build/bin/antwika_life/antwika_life
build/bin/antwika_life/antwika_life --record demo.replay
build/bin/antwika_life/antwika_life --replay src/apps/life/replays/demo.json
```

It opens a window, draws the board each tick, and takes mouse input; drag over cells to toggle them.
It has no end of its own: it runs until the window is closed, or until a replay dispatches `engine.stop`.
A headless build reports neither, so `Ctrl+C` ends one — and since a `--record` run only writes its file once the run ends, an interrupted headless recording saves nothing.

## Libraries it composes

[`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`engine`](../libraries/engine.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`time`](../libraries/time.md), plus the selected backends.

## How it is put together

Each cell is an entity carrying a `Cell` component.
`LifeSystem` advances every cell one generation per tick through the double-buffered `World` and `SystemScheduler`.
`Board`, `Grid` and `BoardSink` handle the scripted `life.toggle_cell` event; `PointerToggleSink` handles the mouse.
`BoardScene`, `RenderSystem` and `PrintSystem` are the write-only output side, and `TickPacer` paces the uncapped run.

## Non-obvious decisions

**A drag is recorded; the toggles it caused are not.**
`input::LiveInputSource` puts each edge into the tick stream and `PointerToggleSink` decodes the `input.pointer_*` events and toggles the cell under the pointer, so a `--record` run persists the click and regenerates the toggle.

**Where a cell is drawn and which cell a click lands in are one function.**
`layoutFor()` and `cellAt()` in `BoardLayout.hpp` are shared by `BoardScene` and `PointerToggleSink`, so the two cannot drift.
That mapping is against the *configured* window size rather than the size a window reports, and the window is not resizable, which is what keeps a recorded session landing on the same cells under a different backend.

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
