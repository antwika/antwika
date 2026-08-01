# apps/game

`src/apps/game/` — an isometric city you build on with the mouse.

## What it demonstrates

That a camera can be simulation state, and that a replay can hold nothing but the clicks.

This is the most complete composition in the project: reducer state, an ECS, an economy, A* inside the tick, live input, a texture atlas, a UI toolbar, save files with a migration, and a renderer that draws faster than the simulation ticks — all hanging off one tick loop.

## Running it

```sh
build/bin/antwika_game/antwika_game
build/bin/antwika_game/antwika_game --record demo.replay
build/bin/antwika_game/antwika_game --replay src/apps/game/replays/demo.json
```

Left-click places whatever the toolbar has selected, right-click drops a walker onto a road, middle-drag pans, and the wheel zooms.
It starts on an empty grid and loads nothing unless `--replay` says so, so a session contains exactly what somebody clicked.

It runs until Escape is pressed or the window is closed — both of which are input, so both are recorded and both replay.
Neither reaches the `null` backend, so a default build runs until interrupted, and a `--record` there never gets to save; use an `sdl3` or `raylib` build (with `SDL_VIDEODRIVER=dummy` or `xvfb-run` if there is no display).

## Libraries it composes

[`animation`](../libraries/animation.md), [`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`pathfinding`](../libraries/pathfinding.md), [`replay`](../libraries/replay.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), plus the selected graphics and input backends.

Notably **not** [`ecs_commons`](../libraries/ecs_commons.md): a walker counts steps rather than ticks, so `Lifetime` would be the wrong shape.

## The three screens

`AppMode` is `MainMenu`, `WorldMap` or the city itself, and `ModeGatedSink`/`ModeGatedSystem` are how a sink or a system is made to run in one mode only.
That is a wrapper rather than a branch inside each sink, so "which mode is this for" is stated once, where the sink is registered.

`WorldMapState` holds one `PathIndex` and one `BuildingIndex` **per city**, while `Building` entities stay global — a distinction worth remembering, because the two would otherwise silently disagree when switching cities.

## The economy

`BuildTool` is the toolbar's palette; `BuildingKind` is the model; `buildingKindOf()` is the one crossing between them.
They used to be one enumeration, which gave every per-building table a `Road` entry that could only ever be wrong.

A house consumes what is delivered to it, and the four sources each send out one `WalkerKind`.
Both facts are arithmetic over the shared declaration order rather than a switch, so a sixth kind is two enumerators and a tile.

`BuildingSystem` runs deliveries, drain, risk and demolition.
Every period derives from one `kTicksPerSecond` rather than a constant per rule, and each countdown lives in the building's own component so two buildings put up a tick apart never fall into lockstep.

## Non-obvious decisions

**The camera is simulation state, not render state.**
A click arrives as a pixel, and which cell it means depends entirely on the camera, so a renderer-owned camera would leave a replay resolving recorded clicks against a different view.
That is also why zoom is an index into a table of whole tile sizes rather than a scale factor, why `floorDiv()` exists instead of `operator/`, and why the projection is anchored to the camera's pan rather than the canvas centre — anchoring to the centre would make a window resize change which cell a pixel means.

**There is no event for placing anything.**
A click is the input; `GridSink` turns it into a placement inside the tick path, and the replay stores the click and regenerates the placement.
Persisting both would lay two tiles per click.
The toolbar defines no event either, for the same reason.

**A building may have one walker out at a time, and it holds the handle.**
Counting walkers per building would be a scan of every walker per building per tick; a handle is a lookup.
The handle is a *cache* and `world.alive()` is the authority, which is safe because `ecs::EntityManager` never reuses an index — so a stale handle can only ever be dead, never somebody else.

`SpawnSystem` needs no new scheduler phase: `destroy()` only retires at `commit()`, so on the tick a walker dies its building still reads it alive and does not spawn, and the building is free from the *next* tick.

**Once a walker's roaming budget is spent it either walks home or it is gone.**
That single rule is what bounds the population, and every awkward case collapses into its last arm — a walker nobody sent, one whose building has burned down, one walled off from home, one whose road was demolished under it.
All four are answered by destroying the walker rather than by four rules, and none of them is an error.
A right-click walker therefore expires too; nothing in the app is immortal.

**The route home is re-searched every step, and only its first move is used.**
A route cannot live in a component — `Component` forbids a `std::vector` — and one held in a system would be state outside the `World` that a save does not cover.
`stepTowards()` is replay-safe because A* orders down to ascending `NodeId`, which is exactly why the extent is passed in rather than derived from the roads: a bounding box computed from whichever roads happen to exist would renumber every node as one was laid, and with it the tie-break.

**Two walkers reaching one building in a tick add up rather than racing.**
Each would otherwise read the same committed amount and stage a write, so the last would win and quietly halve a delivery.
Every change accumulates in a map and each building is written once.

**A building covers a square block, and square is load-bearing.**
`footprintOf()` is a table keyed by kind rather than a field on the component: a field could disagree with the kind that placed it, and the ghost has to know the size *before* any entity exists.

Because a cell's box is twice as wide as it is tall, every square footprint's box comes out 2:1 — the same shape as one atlas tile.
So a square block *is* a diamond, and its art is one ordinary tile scaled up with no geometric error and no atlas work at all.
A 2×3 block is a hexagon that would need a source rect of its own, a half-tile-quantised band in the sheet, and a much heavier contract with whoever draws it.

`canPlace()` is the one statement of what a block will land on, used by `GridSink` *and* by `ghostFor()`, so what a preview promises and what a click delivers cannot drift.
A refused block is shown reddened rather than hidden, since a refusal somebody can see is one they can act on.

**Painter's order stopped being optional.**
Two one-cell buildings could never overlap, so placement order was as good as any; a block drawn before what is behind it is simply the wrong picture.
`snapshotOf()` sorts on `x + y` with a tie-break on `x`, which is screen depth and is total.

**A walker slides between cells, and those frames are drawn outside the tick.**
[`app`](../libraries/app.md)'s `FramePacedSource` draws the extra frames in the gap before a tick's events are read, then hands back what the inner source returned unchanged — so it is a pure observer, and this app's own `TickPacer` is gone because one frame a tick is the same thing it did.

What a frame is handed is an `app::IFramePass`, whose only method takes an `animation::Progress` and **no `World`, no `Tick` and no dispatcher**: a pass between two ticks cannot change what the simulation computes because it is given nothing it could change.

`Walker::from` is **simulation state rather than a render channel**, because a live run and its replay have to agree on it — and it is a `std::optional<Cell>` rather than a cell that lies, since a freshly placed or freshly restored walker has no previous cell.
The picture and the state part company at `SceneSnapshot`: `WalkerSprite` carries `from` and `ticksIntoStep` for drawing, while `WalkerView` stays what `GameSummary` and `SaveGame` hold.

**A button lights up on the press, and the ghost still follows the pointer.**
`input::IdleMotionSource` holds idle movement back, which is why a button does not light up on approach.
The ghost is the exception, and it is not a trade any more: `ghostFor()` works it out on the render side from `input::PointerHintChannel`, which carries a free-moving pointer without putting one byte in a recording.

The ghost is therefore a value the renderer computes each frame, never a component staged into the `World` — a replay does not reproduce the channel, so folding a hint into simulation state would make a run and its replay disagree silently.
No sink may read it, and "the ghost is over the toolbar" is worked out *from* `UiOverlay` rather than the other way round, since `UiOverlay` is derived from recorded input and the hint is not.

**Order of registration is load-bearing.**
`UiSink` is registered *before* `GridSink`, so a press is resolved against the toolbar before the grid sees it.
`UiOverlay` is the one fact the two share and owns the canvas the bar is laid out against — the size the window was *asked* for — so nothing can lay it out against one size and hit-test it against another.
What the bar covers, it covers from the grid too, though not a movement, so a pan begun on the grid carries on across the bar.

**A save is version 2 and carries the buildings.**
Kinds, stock, risk and all three countdowns — countdowns reset on load are exactly the lockstep they exist to avoid.

The building/walker link is persisted **as a pair of array indices rather than as an `ecs::Entity`**, because `EntityManager` hands ids out from a monotonic counter and a restore destroys and recreates everything, so a raw handle would name nothing on the way back in.
Reading refuses an index past the end of the array it points into, and refuses a pair that disagree about each other, rather than repairing either.

Restoring creates every entity *before* adding any component, because `create()` is immediate where `add()` is staged — so a link has to be built into the component rather than written onto it afterwards.

**Walkers do not collide.**
Two may occupy one cell, because nothing requires otherwise and a rule to avoid it would be a requirement nobody asked for.

**The atlas is hand-drawn art, and `TileAtlas.hpp` is its address map.**
`assets/atlas.png` used to be generated by a script CI re-ran to prove it had not drifted; the art is now drawn and curated directly, so nothing rebuilds it and editing it is editing the art — see [`game-texture-atlas.md`](game-texture-atlas.md).
The header says where a tile lives arithmetically rather than as a table of rectangles, so repainting a tile is free and moving one is not.
What is left to catch a mistake is that header's `static_assert`s, `TileAtlasTest`, and a check at startup that the PNG really is the expected size.

Which of the sixteen road tiles a junction shows is worked out in `GridScene` by binary-searching the snapshot's ascending paths, and stays out of `SceneSnapshot` and `GameSummary` — it is a picture, not state a replay has to reproduce.

## See also

- [`blog/013-the-camera-is-simulation-state.md`](../../blog/013-the-camera-is-simulation-state.md)
- [`blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md`](../../blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md)
- [`game-texture-atlas.md`](game-texture-atlas.md) — what an artist has to produce.
