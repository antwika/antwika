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
With the road tool selected a left-drag lays a whole run of road: the press marks where it starts, the pointer says where it ends, and the release lays the route between them.
F10, or the toolbar's `menu` button, opens a menu modal over the city with two items: one back to the main menu, and one back to the game.
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

**A road is dragged out rather than clicked one cell at a time, and that is no more an event than one click is.**
A recording holds the press, the movements and the release; `RoadDrag` is where the gesture's start and end live, and `planRoad()` is what says how the one gets to the other.
That plan is an A* through [`pathfinding`](../libraries/pathfinding.md) on exactly `stepTowards()`'s terms — ties break down to ascending `NodeId`, and the extent is passed in rather than derived from what happens to exist, since a bounding box taken off the roads would renumber every node as one was laid.
`RoadDrag` is therefore simulation state in the camera's sense, written by `GridSink` inside the tick path and never from `input::PointerHintChannel`.
The pressed cell is laid at once rather than at the release, so a plain click stays the single-tile placement it always was, and a recording that holds no release lays exactly what it always laid.

Three decisions are worth stating outright.
**Only roads are dragged**: a run of houses is not a route, so a path search says nothing about where one would go, and a building tool would need a rule of its own about what a rectangle of blocks means.
**A drag holds the run still and lets it go again**, so a route cannot be planned against a city moving under it — but only when the drag was what held it, so a drag never resumes a run somebody paused for themselves (`RoadDrag::heldForDrag()`).
**A route that does not exist builds nothing at all**: the preview shows the two cells that were named, reddened, which is the convention `canPlace()` and the build ghost already follow, and half a route is a road to nowhere nobody asked for.

The preview itself rides on `SceneSnapshot::plan`, and unlike the ghost beside it that member is filled in once a tick rather than once a frame — it is derived from state a replay reproduces, so there is nothing about it a frame could see that the tick did not.

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

**Those frames carry on while the run is paused, which is why the pause reaches the picture too.**
`SceneSnapshot` carries it, read off `PauseState` by `snapshotOf()` exactly as the camera is, and `GridScene` then draws a held walker at its step's own phase whatever fraction of a tick a frame falls at.
Without that a walker frozen mid-step slid forward through every tick and snapped back at the start of the next one, for as long as the run stayed held — the whole ticks of its step stop with `WalkerSystem`, and `FramePacedSource`'s frames do not stop with them.
The decision is the scene's rather than whoever supplies the fraction, so a held snapshot is the same picture wherever it is drawn.

**A button lights up on the press, and the ghost still follows the pointer.**
`input::IdleMotionSource` holds idle movement back, which is why a button does not light up on approach.
The ghost is the exception, and it is not a trade any more: `ghostFor()` works it out on the render side from `input::PointerHintChannel`, which carries a free-moving pointer without putting one byte in a recording.

The ghost is therefore a value the renderer computes each frame, never a component staged into the `World` — a replay does not reproduce the channel, so folding a hint into simulation state would make a run and its replay disagree silently.
No sink may read it, and "the ghost is over the toolbar" is worked out *from* `UiOverlay` rather than the other way round, since `UiOverlay` is derived from recorded input and the hint is not.

**Order of registration is load-bearing.**
`UiSink` is registered *before* `GridSink`, so a press is resolved against the toolbar before the grid sees it.
`UiOverlay` is the one fact the two share and owns the canvas the bar is laid out against — the size the window was *asked* for — so nothing can lay it out against one size and hit-test it against another.
What the bar covers, it covers from the grid too, though not a movement, so a pan begun on the grid carries on across the bar.

**The menu modal is a modal rather than a mode, and whether it is up is simulation state.**
F10 and the toolbar's `menu` button both open it, `UiSink` owns the flag, and no `ui.*` or `game.*` event exists for any of it — a recording holds the key press and the click, and a replay works out again which widget they hit.
Its scene is `MenuModalScene`, and the scrim behind the card is load-bearing rather than decoration: it is a container the size of the whole canvas with a fill behind it, so `ui::Interactions::pointerOverUi` is true wherever the pointer is and `GridSink`'s existing "what the UI covers, it covers from the grid too" rule keeps every press off the city with no second mechanism invented for it.
The modal's commands are appended after the bar's in the one `UiOverlay` the renderer paints last, which is how "on top" is said where `antwika::gfx` offers no depth but paint order, and a press is resolved against the modal alone so a toolbar button cannot be pressed through it.

Three awkward cases have rules.
**Opening it ends a road drag in progress, and that drag lays nothing**: what a drag lays is what its release said, and a release arriving over the modal never said it.
**Opening it holds the run**, exactly as `CityEntrySink` holds one on entering a city — `hold()` rather than `toggle()`, for that sink's reason — and closing it does not let the run go again, so the way out is the pause button it always was.
That also settles the drag, since the modal's hold supersedes whatever the drag was holding and nothing here ever resumes.
**Leaving for the main menu is a mode change like every other**, asked for on `AppModeState` so it lands at the tick boundary, and the modal is put away on the way out so a city entered later is not still wearing it.

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
