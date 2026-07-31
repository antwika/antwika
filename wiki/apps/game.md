# apps/game

`src/apps/game/` — an isometric grid you build on with the mouse.

## What it demonstrates

That a camera can be simulation state, and that a replay can hold nothing but the clicks.
This is the most complete composition in the project: reducer state, an ECS, live input, a texture atlas, a UI toolbar and a renderer, all hanging off one tick loop.

## Running it

```sh
build/bin/antwika_game
build/bin/antwika_game --record demo.replay
build/bin/antwika_game --replay src/apps/game/replays/demo.json
```

Left-click lays a path tile, right-click drops a walker onto one, middle-drag pans, and the wheel zooms.
It starts on an empty grid and loads nothing unless `--replay` says so, so a session contains exactly what somebody clicked.
It runs until Escape is pressed or the window is closed — both of which are input, so both are recorded and both replay.
Neither reaches the `null` backend, so a default build runs until interrupted, and a `--record` there never gets to save; use an `sdl3` or `raylib` build (with `SDL_VIDEODRIVER=dummy` or `xvfb-run` if there is no display).

## Libraries it composes

[`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`engine`](../libraries/engine.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), plus the selected graphics and input backends.

## How it is put together

- `GameState` and `GameStateReducer` fold `game.score_increment` — the app's only event name — through [`reducer`](../libraries/reducer.md).
- `Camera`, `IsoProjection.hpp`, `GridExtent`, `Cell`, `Path`, `PathIndex` hold the grid and the view.
- `GridSink` turns pointer events into placements, pans and zooms; `UiSink` describes and resolves the toolbar; `InputFold` and `PointerReading` fold edges into a pointer.
- `WalkerSystem` advances each `Walker` one cell per tick; `nextFacing()` in `Walking.hpp` is one preference order.
- `SceneSnapshot`, `GridScene`, `Toolbar`, `UiOverlay` and `RenderSystem` are the write-only render side; `TileAtlas.hpp` addresses `assets/atlas.png`.
- `WindowInputSource` and `TickPacer` are the app's own thin wrappers around the `replay` versions.

## Non-obvious decisions

**The camera is simulation state, not render state.**
A click arrives as a pixel, and which cell it means depends entirely on the camera, so a renderer-owned camera would leave a replay resolving recorded clicks against a different view.
That is also why zoom is an index into a table of whole tile sizes rather than a scale factor, why `floorDiv()` exists instead of `operator/`, and why the projection is anchored to the camera's pan rather than the canvas centre — anchoring to the centre would make a window resize change which cell a pixel means.

**There is no event for placing anything.**
A click is the input; `GridSink` turns it into a placement inside the tick path, and the replay stores the click and regenerates the placement.
Persisting both would lay two tiles per click.
The toolbar defines no event either, for the same reason.

**Order of registration is load-bearing.**
`UiSink` is registered *before* `GridSink`, so a press is resolved against the toolbar before the grid sees it.
`UiOverlay` is the one fact the two share and owns the canvas the bar is laid out against — the size the window was *asked* for — so nothing can lay it out against one size and hit-test it against another.
What the bar covers, it covers from the grid too: `GridSink` skips a press or a scroll the overlay reports as covered, though not a movement, so a pan begun on the grid carries on across the bar.

**Walkers do not collide.**
Two may occupy one cell, because nothing requires otherwise and a rule to avoid it would be a requirement nobody asked for.

**Buttons light up on the press, not on approach.**
That is `input::IdleMotionSource` in this app's chain rather than anything [`ui`](../libraries/ui.md) decides: idle pointer movement is held back until something reads it.
Clicking is unaffected, since the gate releases the latched movement ahead of the press and a press carries its own position.
Taking the gate out of `main.cpp` would buy live hover back at the recording size it was added to save.

**The atlas is generated, and CI checks it.**
[`scripts/generate_game_atlas.py`](../../scripts/generate_game_atlas.py) draws `assets/atlas.png` from the same slot numbers `TileAtlas.hpp` addresses it with, in grid space, so a road stub's shape falls out of the same projection the game blits it through.
Which of the sixteen road tiles a junction shows is worked out in `GridScene` by binary-searching the snapshot's ascending paths, and stays out of `SceneSnapshot` and `GameSummary` — it is a picture, not state a replay has to reproduce.

See [`blog/013-the-camera-is-simulation-state.md`](../../blog/013-the-camera-is-simulation-state.md).
