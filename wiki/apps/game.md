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

Left-click places whatever the toolbar has selected, middle-drag pans, and the wheel zooms.
Right-click means one of two things: with a building tool selected it puts the palette down and places nothing by that press, and otherwise it drops a walker onto the road under the pointer.
With the road tool selected a left-drag lays a whole run of road: the press marks where it starts, the pointer says where it ends, and the release lays the route between them.
The toolbar carries zoom, reset-view, pause and menu buttons, drawn over the grid by `Toolbar`, described and resolved once per tick by `UiSink`, and painted last by `RenderSystem`; the bar also reports the tick, and a corner of the screen reports the frame rate.
A city runs from the moment it comes up, so that button is how a player asks for a pause and how they let one go.
F10, or the toolbar's `menu` button, opens a menu modal over the city with two items: one back to the main menu, and one back to the game.
It starts on an empty grid and loads nothing unless `--replay` says so, so a session contains exactly what somebody clicked.

It runs until Escape is pressed or the window is closed — both of which are input, so both are recorded and both replay.
Neither reaches the `null` backend, so a default build runs until interrupted, and a `--record` there never gets to save; use an `sdl3` or `raylib` build (with `SDL_VIDEODRIVER=dummy` or `xvfb-run` if there is no display).

## Libraries it composes

[`animation`](../libraries/animation.md), [`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`pathfinding`](../libraries/pathfinding.md), [`replay`](../libraries/replay.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), plus the selected graphics and input backends.

Notably **not** [`ecs_commons`](../libraries/ecs_commons.md): a walker counts steps rather than ticks, so `Lifetime` would be the wrong shape.

## The three screens

`AppMode` is `MainMenu`, `WorldMap`, the city itself or the save picker, and `ModeGatedSink` is how a sink is made to run in one mode only.
That is a wrapper rather than a branch inside each sink, so "which mode is this for" is stated once, where the sink is registered.

A *system* is gated differently, by `SessionGatedSystem`, which asks `simulates()` rather than naming a mode: **a city runs whether or not anybody is looking at it**, so the walkers, the buildings and the spawns carry on while whoever is playing reads the world map.
They stop only on the two screens where there is no session to run — the main menu and the save picker.
A city stopping because somebody opened a screen was a pause nobody asked for, and asking is now the only thing that holds a run still.

`WorldMapState` holds one `PathIndex` and one `BuildingIndex` **per city**, while `Building` entities stay global — a distinction worth remembering, because the two would otherwise silently disagree when switching cities.

## The economy

`BuildTool` is the toolbar's palette; `BuildingKind` is the model; `buildingKindOf()` is the one crossing between them.
They used to be one enumeration, which gave every per-building table a `Road` entry that could only ever be wrong.

A house consumes what is delivered to it, and most other kinds send out one `WalkerKind`.
Both facts used to be arithmetic over a shared declaration order; they are tables now, for the reason the round-one vocabulary section below gives.

`BuildingSystem` runs deliveries, drain, risk and demolition.
Every period derives from one `kTicksPerSecond` rather than a constant per rule, and each countdown lives in the building's own component so two buildings put up a tick apart never fall into lockstep.

A building with no road beside it holds its countdown at zero rather than resetting it, so laying a road beside a long-neglected source releases one walker and not a queue of them, and `kWalkerLimit` stays as a backstop.

`BuildingIndex` is `PathIndex`'s counterpart and exists for the same reason, with two writers and only two: `GridSink` records a block as it builds on one, and `BuildingSystem` clears one as it demolishes.

## Non-obvious decisions

**The reducer state did not go away when the ECS arrived.**
The plain `GameState` struct and its `GameStateReducer` are still there, folding `game.score_increment` alongside the grid.

**The camera is simulation state, not render state.**
A click arrives as a pixel, and which cell it means depends entirely on the camera, so a renderer-owned camera would leave a replay resolving recorded clicks against a different view.
That is also why zoom is an index into a table of whole tile sizes rather than a scale factor, why `floorDiv()` exists instead of `operator/`, and why the projection is anchored to the camera's pan rather than the canvas centre — anchoring to the centre would make a window resize change which cell a pixel means.

**There is no event for placing anything.**
A click is the input; `GridSink` turns it into a placement inside the tick path, and the replay stores the click and regenerates the placement.
Persisting both would lay two tiles per click.
The toolbar defines no event either, for the same reason.

**A right press means one of two things, and the palette decides which.**
While a building tool is selected it leaves build mode, putting the palette *down* and placing nothing by that press; otherwise it drops a walker on the path under the pointer exactly as it always did.
So `UiOverlay::tool()` is a `std::optional<BuildTool>`, and nothing selected is a state the app can be in rather than a synonym for the road tool: no button on the bar is held down, `ghostFor()` returns an invisible ghost so neither a preview tile nor its `footprintOutline()` border is drawn, and a left press lays nothing at all.
It used to fall back to `BuildTool::Road`, which made cancelling twice the same as cancelling once at the price of a cancel that quietly armed a different tool.
Putting the palette down says what a cancel means, and a right press with nothing selected still drops a walker, so cancelling twice is still cancelling once.
The whole decision is `GridSink`'s rather than split with `UiSink`: `UiSink` runs first, so a cancel resolved there would leave the selection cleared by the time the grid read that same press, and one press would then cancel *and* drop a walker.
Leaving build mode is no more an event than laying a tile is — a recording holds the right press, and a replay resolves it against the same selection and arrives at the same one.

**A road is dragged out rather than clicked one cell at a time, and that is no more an event than one click is.**
A recording holds the press, the movements and the release; `RoadDrag` is where the gesture's start and end live, and `planRoad()` is what says how the one gets to the other.
That plan is an A* through [`pathfinding`](../libraries/pathfinding.md) on exactly `stepTowards()`'s terms — ties break down to ascending `NodeId`, and the extent is passed in rather than derived from what happens to exist, since a bounding box taken off the roads would renumber every node as one was laid.
The roads are therefore not consulted at all: an existing one is passable and simply not laid again, and what a route may not cross is a building, which keeps the plan a function of strictly less state than the placement it feeds.
`RoadDrag` is therefore simulation state in the camera's sense, written by `GridSink` inside the tick path and never from `input::PointerHintChannel`.
The pressed cell is laid at once rather than at the release, so a plain click stays the single-tile placement it always was, and a recording that holds no release lays exactly what it always laid.

Three decisions are worth stating outright.
**Only roads are dragged**: a run of houses is not a route, so a path search says nothing about where one would go, and a building tool would need a rule of its own about what a rectangle of blocks means.
**A drag holds nothing still**, so a route is planned against a city that goes on moving under it.
It used to pause the run for exactly that reason, and a city now runs all the time unless a player has asked otherwise.
**A route that does not exist builds nothing at all**: the preview shows the two cells that were named, reddened, which is the convention `canPlace()` and the build ghost already follow, and half a route is a road to nowhere nobody asked for.

A fresh press ends whatever gesture preceded it and lays none of its route, since what a drag would lay is what its release said and no release ever said it.

The preview itself rides on `SceneSnapshot::plan`, and unlike the ghost beside it that member is filled in once a tick rather than once a frame — it is derived from state a replay reproduces, so there is nothing about it a frame could see that the tick did not.

**A building may have one walker out at a time, and it holds the handle.**
Counting walkers per building would be a scan of every walker per building per tick; a handle is a lookup.
The handle is a *cache* and `world.alive()` is the authority, which is safe because `ecs::EntityManager` never reuses an index — so a stale handle can only ever be dead, never somebody else.

`SpawnSystem` needs no new scheduler phase: `destroy()` only retires at `commit()`, so on the tick a walker dies its building still reads it alive and does not spawn, and the building is free from the *next* tick.

**A walker's step is counted down in its own component rather than off the tick number.**
It advances one cell every `kTicksPerStep` ticks along the paths, and counting in the component is what keeps two walkers that set off a tick apart a tick apart, exactly as a building's countdown does.
Where it goes next is one preference order in `nextFacing()` rather than two rules: it prefers a right turn at an intersection and reverses at a dead end, and both fall out of that single order.

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

**The block a click would take is outlined as well as tinted**, by `footprintOutline()` — four points traced round the very `footprintBounds()` box the ghost tile is blitted into, so a border and the preview it surrounds cannot show two different extents.
It is four `IRenderer::drawLine()` calls rather than four filled rectangles: a square block is a diamond, its edges are diagonal, and `drawRect()` takes an upright box only — which is why `antwika::ui`'s focus ring can be four fills and this cannot.
Nothing reads a drawn line back, so which pixels a backend lights between two endpoints cannot reach anything a replay reproduces, and the border stays exactly as render-side as the ghost it is drawn round.

Two consequences fall out of blocks that one-cell buildings never had.

**Painter's order stopped being optional.**
Two one-cell buildings could never overlap, so placement order was as good as any; a block drawn before what is behind it is simply the wrong picture.
`snapshotOf()` sorts on `x + y` with a tie-break on `x`, which is screen depth and is total.

**And a walker reaches a building by *any* cell of its block.**
That is why `spawnCellFor()` walks the whole perimeter and `stepTowards()` makes every cell of the goal passable.
There is deliberately no guard against one walker serving one building twice: two of a cell's four neighbours being in one rectangle would put the cell in it too, so it would be a road under a building, which nothing places.

**A walker slides between cells, and those frames are drawn outside the tick.**
[`app`](../libraries/app.md)'s `FramePacedSource` draws the extra frames in the gap before a tick's events are read, then hands back what the inner source returned unchanged — so it is a pure observer, and this app's own `TickPacer` is gone because one frame a tick is the same thing it did.

What a frame is handed is an `app::IFramePass`, whose only method takes an `animation::Progress` and **no `World`, no `Tick` and no dispatcher**: a pass between two ticks cannot change what the simulation computes because it is given nothing it could change.

`RenderSystem` implements both interfaces, snapshotting in `update()` and redrawing that snapshot in `draw()`, and that cached `SceneSnapshot` is the app's only render-side mutable state — safe for the one reason above, so handing `draw()` a `World` would quietly remove the guarantee.

`Walker::from` is **simulation state rather than a render channel**, because a live run and its replay have to agree on it and both draw the same picture from it.
Reconstructing it as `step(at, opposite(facing))` is right mid-run and wrong exactly where there was no previous cell — freshly placed, freshly spawned, restored from a save — which is why it is a `std::optional<Cell>` rather than a cell that lies.
The picture and the state part company at `SceneSnapshot`: `WalkerSprite` carries `from` and `ticksIntoStep` for drawing, while `WalkerView` stays what `GameSummary` and `SaveGame` hold, since a render-only field would otherwise land in a persisted schema and in the value `ReplayDeterminismTest` compares — the same rule that keeps a road's link mask out of the snapshot.

The interpolation itself is `WalkerMotion.hpp`, exact rational arithmetic through `animation::interpolate`, so the same frame of the same tick is the same pixel on every toolchain.
`FrameRateDeterminismTest` is what pins that drawing more often cannot change what a run computes.

**The pause is `apps/life`'s answer to the same question.**
`PauseGatedSystem` wraps a system and stages nothing while `PauseState` says the run is held, exactly as `life::DragPausedSystem` does, and only those systems stop.
Which ones is the product decision, and it is three: `WalkerSystem`, `BuildingSystem` and `SpawnSystem`, the three that make a city move on its own.
The tick, the commit, every observer, the toolbar, the camera and placement all carry on, so **a paused city can still be panned over, zoomed into and built on** — this is a build pause rather than a freeze, and a pause nobody could act on would just look like a hang.
It composes with `SessionGatedSystem` rather than replacing it, since a run is paused *and* has a session on screen and either gate alone answers only its own question.

`PauseState` is simulation state in exactly the sense the camera and the selected tool are — owned by `main.cpp` beside those two for the reason all three are, since a renderer built before the run has to read it — and written by `UiSink` inside the tick path, so a replay pauses on precisely the ticks the live run paused on and nothing about a pause is ever persisted.

The slide itself is `WalkerMotion.hpp`, through [`tween`](../libraries/tween.md) at `kWalkerEasing` — exact rational arithmetic either way, so the same frame of the same tick is the same pixel on every toolchain, which `FrameRateDeterminismTest` pins.
That constant is `Easing::Linear` **on purpose rather than pending**: a walker crosses many cells in a row, so easing each cell's step would make it start and stop at every tile.
Easing the camera that follows it is the version of that idea which reads correctly.

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
What the bar covers, it covers from the grid too — `GridSink` skips a press or a scroll the overlay reports as covered, though not a movement, so a pan begun on the grid carries on across the bar.
A release is exempt for the reason a movement is plus one of its own: a drag let go over the bar would otherwise hold the pause for the rest of the session.

**The bar reports the tick and a corner of the screen reports the frame rate, and the two are in different places for one reason.**
The tick is simulation state, so it is a label on the bar, described in the tick path off the `TickEvent` being handled and regenerated by a replay like everything else there.
Frames per second is measured against a wall clock, which says how fast the machine is — so `FrameMeter` is render-side only, is handed an injected `time::IClock` rather than reading one, is counted in `RenderSystem::draw()` (the one thing that runs exactly once per frame, `FramePacedSource`'s between-tick frames included), and is drawn by `describeFps()` straight from the renderer.
It reaches no sink, no system, no `SceneSnapshot`, no `GameSummary` and no save, which is the whole of its safety condition — the same one `input::PointerHintChannel` is held to, arrived at from the other side.
`RenderSetup::fps` is optional and absent by default, so a run with no wall clock to offer draws no readout and every test whose subject is the picture is spared one.

**What the first second of a run shows is a placeholder rather than a number.**
`FrameMeter::perSecond()` answers nothing at all until a whole window has gone by, and `describeFps()` draws `kNoRateReadout`, which reads `fps --`, in its place.
That constant is the whole line rather than the two dashes alone, and the readout is two `ui.label()` calls rather than one over a conditional string, because a caption joined to a placeholder is a `std::string` built on a branch — a temporary whose unwind path no test can reach, and two branches the coverage gate then refuses.
Absent rather than zero, because zero is a rate a stalling machine is genuinely measured at, and a run that has drawn for less than a second has measured nothing; reporting both as "fps 0" would be one word for two states, and the corner would also claim the machine was drawing no frames at the exact moment it started drawing them.

**The menu modal is a modal rather than a mode, and whether it is up is simulation state.**
F10 and the toolbar's `menu` button both open it, `UiSink` owns the flag, and no `ui.*` or `game.*` event exists for any of it — a recording holds the key press and the click, and a replay works out again which widget they hit.
Its scene is `MenuModalScene`, and the scrim behind the card is load-bearing rather than decoration: it is a container the size of the whole canvas with a fill behind it, so `ui::Interactions::pointerOverUi` is true wherever the pointer is and `GridSink`'s existing "what the UI covers, it covers from the grid too" rule keeps every press off the city with no second mechanism invented for it.
The modal's commands are appended after the bar's in the one `UiOverlay` the renderer paints last, which is how "on top" is said where `antwika::gfx` offers no depth but paint order, and a press is resolved against the modal alone so a toolbar button cannot be pressed through it.

Three awkward cases have rules.
**Opening it ends a road drag in progress, and that drag lays nothing**: what a drag lays is what its release said, and a release arriving over the modal never said it.
**Opening it holds nothing**, and neither does entering a city: the city goes on behind the modal exactly as it goes on behind the world map.
A run is held where a player has asked for it and nowhere else — see the pause below.
**Leaving for the main menu is a mode change like every other**, asked for on `AppModeState` so it lands at the tick boundary, and the modal is put away on the way out so a city entered later is not still wearing it.

The modal's widget ids live in `modalWidgets` rather than in the bar's `widgets`, for the reason `menuWidgets` has a namespace of its own: the two are resolved against different frames and never share one.

**A pause is asked for, and nothing else causes one.**
`PauseGatedSystem` wraps a system and stages nothing while `PauseState` says the run is held, exactly as `life::DragPausedSystem` does, and the three it wraps are `WalkerSystem`, `BuildingSystem` and `SpawnSystem` — the three that make a city move on its own.
The tick, the commit, every observer, the toolbar, the camera and placement all carry on, so **a paused city can still be panned over, zoomed into and built on**: this is a build pause rather than a freeze, and a pause nobody could act on would just look like a hang.

A city coming up, the menu modal opening and a road being dragged out each used to hold the run as well, and none of them does.
That deleted `CityEntrySink` outright and left `PauseState` one writer rather than four, and it removed the bookkeeping about *whose* pause a release was letting go of.
What it buys beyond the simplification is that opening a screen is no longer a pause nobody asked for — which matters most where more than one player shares a city, since one of them reading the map must not stop it for the others.

**The value is absolute rather than a toggle**, which is the same decision read forward.
`PauseState::set(bool)` is idempotent, and the button sends the opposite of the state it was showing rather than flipping one.
Two players pausing on one tick therefore agree on a pause, where two toggles would have cancelled out and left the run going with both of them having watched themselves press pause.

**A save is version 2 and carries the buildings.**
Kinds, stock, risk and all three countdowns — countdowns reset on load are exactly the lockstep they exist to avoid.

The building/walker link is persisted **as a pair of array indices rather than as an `ecs::Entity`**, because `EntityManager` hands ids out from a monotonic counter and a restore destroys and recreates everything, so a raw handle would name nothing on the way back in.
Reading refuses an index past the end of the array it points into, and refuses a pair that disagree about each other, rather than repairing either — a repaired save is a session somebody never had.

Restoring creates every entity *before* adding any component, because `create()` is immediate where `add()` is staged — so a link has to be built into the component rather than written onto it afterwards.

**Walkers do not collide.**
Two may occupy one cell, because nothing requires otherwise and a rule to avoid it would be a requirement nobody asked for.

**The atlas is hand-drawn art, and `TileAtlas.hpp` is its address map.**
`assets/atlas.png` used to be generated by a script CI re-ran to prove it had not drifted; the art is now drawn and curated directly, so nothing rebuilds it and editing it is editing the art — see [`game-texture-atlas.md`](game-texture-atlas.md).
The header says where a tile lives arithmetically rather than as a table of rectangles, so repainting a tile is free and moving one is not.
What is left to catch a mistake is that header's `static_assert`s, `TileAtlasTest`, and a check at startup that the PNG really is the expected size.

Which of the sixteen road tiles a junction shows is worked out in `GridScene` by binary-searching the snapshot's ascending paths, and stays out of `SceneSnapshot` and `GameSummary` — it is a picture, not state a replay has to reproduce.

## Resource bars and the hover readout

**What a building depends on and what a walker carries are drawn as small vertical bars, and hovering either says the same thing in words.**

`BuildingSprite` is `BuildingView`'s render-side twin for exactly `WalkerSprite`'s reason: it carries the stock a gauge is drawn from, and `GameSummary` keeps the plain view, so a number that exists only to be looked at stays out of the value `ReplayDeterminismTest` compares.
`buildingViewsOf()` is what a summary is built from now, and `WalkerSprite` gained the walker's kind and load on the same terms.

A bar is a track and a fill in `ResourceBar.hpp`, worked out from `footprintBounds()` and `walkerBounds()` — the very boxes the sprite is blitted into — so the gauges cannot become a second layout that drifts from the art.
They are painted in a pass *after* every sprite, so nothing standing in front of what they gauge can hide one.

Which bars a building shows is `consumes(kind)`: every resource for a house and none for anything else, since a producer keeps stock nobody drains and a gauge on one would count a number that never moves.
A walker shows the one resource `carriedResource()` names, empty bar included, and a walker whose kind carries nothing fixed shows none.

**Hovering is `hoverFor()`, and it is `ghostFor()`'s sibling in every way that matters**: it reads `input::PointerHintChannel`, it resolves the pixel through the same `screenToCell()` a click goes through, it tests a building across its whole block, and what it answers may decide what is drawn and nothing else.

`readoutPanel()` lays that answer out into a plain value of a box and coloured lines, painted through `IRenderer` rather than through `antwika::ui` — deliberately, because this app's UI is described and resolved inside the tick path by `UiSink`, and taking a panel driven by an unrecorded hint through that path is precisely what the channel forbids.
The panel lists the resources the bars gauge rather than every number a building holds, so a reader is never told two stories about one building.
Its captions are a table of their own rather than the names a save file writes, since a persisted name may not change to suit a caption.

`HoverTest` runs one recorded stream twice, with and without a pointer over the grid, and asserts the same `GameSummary` out of both — and that the watched run really did draw a readout, so the two cannot agree for the wrong reason.

## The round-one vocabulary, and why every crossing is a table now

**This is the change that had to land before anything else could be built on it**, and it adds no gameplay of its own: the enumerations grew, three arithmetic identities became tables, a building gained room for a second walker, the captions started going through [`i18n`](../libraries/i18n.md), and the save format took its one bump to version 3.

`Resource` is `Food`, `Clay` and `Pottery`.
**Water left, and that is the load-bearing part.** A good is an amount that moves from one building to another, and what one gains the other loses; a service is a state a walker confers on what it passes, and a well is no poorer for having watered a house.
Water as a good needed a delivery per house per drain; as `Service::Water` it needs a walker to keep *reaching* a district, which is the thing a road network is actually for.
`Service.hpp` names the four -- water, health, safety and structure -- and nothing in this increment reads them beyond naming them: publishing the enumeration first is what lets a coverage component and the systems over it be written against something fixed.

`sustains()` is the other half of that split.
Before, a house was lost when it ran out of *anything* it held, which was exact while both goods were things a walker handed over.
Clay is an industrial input a house never sees, and pottery decides how well a household lives rather than whether it lives at all, so a table saying which good a house cannot go without is what keeps "runs out and is lost" from meaning "is lost the moment it is built".

`BuildingKind` is ten kinds and `WalkerKind` is seven, and **three identities that used to be arithmetic are now tables**:

- `consumes()` was `kind == House`, which stops being exact the moment a workshop eats clay to make pottery.
- `sendsWalkers()` was its negation, which a storehouse breaks outright: goods are carted to it and carted away again, and it sends nobody.
- `buildingKindOf()` and `walkerSentBy()` were offsets into a shared declaration order, exact only while every tool placed a building and every sender sent one walker.

None of those was wrong before.
Each was exact for a reason that round one removes, and the failure mode they share is the bad one: an offset past a hole still lands on a valid enumerator, so the answer is wrong and nothing says so.
Two `static_assert`s hold the tables to each other -- every kind has exactly one tool, and `sendsWalkers()` agrees with `walkerSentBy()` on every kind.

**A building holds `kMaxWalkersOut` walker handles rather than one**, because a market sends a buyer and a seller and a workshop hauls its output away while it waits on a delivery.
It is a fixed `std::array` rather than a vector, since `ecs::Component` wants a trivially copyable, standard-layout type.
A slot number is not a role: a walker goes into the lowest free one, so two buildings that have sent the same walkers in the same order hold them identically.

**A slot is capacity rather than leave to send another.**
`SpawnSystem`'s cadence keeps one walker *of the kind it sends* out at a time, which is what stops a wider array from doubling every building's output on the day it grew; the remaining slot is room for an errand another system sends.
`world.alive()` is still the authority over every handle, and `ecs::EntityManager` never reusing an index is still why a stale one can only be dead.

**Every caption now goes through [`i18n`](../libraries/i18n.md)**, and the locale is fixed at `kDefaultLocale` in `main()`.
That is not a preference: an `antwika::ui` layout is a function of the strings declared into it and a hit-test is a function of that layout, so a run recorded in one language and replayed in another would resolve one recorded click to a different button.
`toolLabel()` and `pauseLabel()` return a `MessageId` rather than words, and `Toolbar`, `GridScene`, `MainMenuScene`, `MenuModalScene` and `SaveLoadScene` take a `const Translator &` like any other injected collaborator.
`buildingKindName()` deliberately does *not*: that is the name a save file writes, and a schema is not something a person reads.

**The save format went to version 3, and `SaveMigrationV2ToV3.cpp` is the one step.**
All three of its changes are genuinely breaking rather than additive -- the walker link became a list, `stock` changed width, and the kind names changed -- which is precisely why they were made together and why the increments after this one need no bump at all.
A version 2 stock of `[food, water]` reads as `[food, 0, 0]`: the food is what it was, the water was never a good, and nobody has carted anything to that building yet.
`SaveGame.cpp` is now a spine that states the document's shape once, with `src/SaveSections.hpp` declaring the pieces it is assembled from -- so a later slice of the format is a file of its own plus three lines in the spine, rather than an edit in the middle of a six-hundred-line function.

## Future work

**`UiSink`/`UiOverlay`/`Toolbar` should adopt `ui::applyHover()` next.**
The app already owns a hint channel and already draws its placement ghost from it, so the toolbar buttons lighting up on approach is `main.cpp` handing `RenderSystem` the channel and one `applyHover()` call after the sink has resolved the press — and it is the one remaining thing `apps/game` says it does not do.

## See also

- [`blog/013-the-camera-is-simulation-state.md`](../../blog/013-the-camera-is-simulation-state.md)
- [`blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md`](../../blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md)
- [`game-texture-atlas.md`](game-texture-atlas.md) — what an artist has to produce.
