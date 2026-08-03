# apps/game

`src/apps/game/` — an isometric city you build on with the mouse.

## What it demonstrates

That a camera can be simulation state, and that a replay can hold nothing but the clicks.

This is the most complete composition in the project: reducer state, an ECS, an economy, A* inside the tick, live input, a texture atlas, a UI toolbar, save files with a migration, and a renderer that draws faster than the simulation ticks — all hanging off one tick loop.

## Running it

```sh
build/bin/antwika_game/antwika_game
build/bin/antwika_game/antwika_game --record demo.replay
build/bin/antwika_game/antwika_game --replay src/apps/game/replays/demo.jsonl
```

Left-click places whatever the palette has selected, middle-drag pans, and the wheel zooms.
Right-click means one of two things: with a building tool selected it puts the palette down and places nothing by that press, and otherwise it drops a walker onto the road under the pointer.
With the road tool selected a left-drag lays a whole run of road: the press marks where it starts, the pointer says where it ends, and the release lays the route between them.
`Toolbar` draws three pieces round the grid, all described and resolved once per tick by `UiSink` and painted last by `RenderSystem`: a strip along the top, a build palette down the right, and a strip along the bottom carrying the view controls and the readouts.
The bottom strip is zoom in, zoom out, reset view and pause, and then the zoom level, the tick, the population and the share of jobs that are staffed; a corner of the screen reports the frame rate separately, because that is a wall clock's answer rather than the simulation's.
A city runs from the moment it comes up, so the pause button is how a player asks for a pause and how they let one go.
The top strip carries a `game` menu — new game, save game, load game, main menu, world map — and the `menu` button that opens the menu modal over the city, with one item back to the main menu and one back to the game.
F10 fills the screen with the window and puts it back, which is the one key here that reaches no sink at all.
The toolbar carries zoom, reset-view, pause and menu buttons, drawn over the grid by `Toolbar`, described and resolved once per tick by `UiSink`, and painted last by `RenderSystem`; the bar also reports the tick, and a corner of the screen reports the frame rate.
A city runs from the moment it comes up, so that button is how a player asks for a pause and how they let one go.
The toolbar's `menu` button opens a menu modal over the city with two items: one back to the main menu, and one back to the game.
The main menu's `Options` button opens the key bindings screen, where an action is picked and the next key pressed is bound to it; Space pauses, `=` and `-` zoom and Home puts the view back, until somebody says otherwise.
F10 fills the screen with the window and puts it back, which — with Escape — is one of the two keys here that reach no sink at all, and therefore the two that may not be bound to anything.
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

**The options screen is a fourth screen without being a fourth mode.**
It is the main menu with something else on it: `OptionsState::open()` says which of the two is up, `MainMenuSink` describes whichever it is into the one `menuOverlay`, and `MainMenuScene::draw()` paints what it is handed either way.
A mode of its own would have wanted a fourth overlay, a fourth arm in `RenderSystem` and a second backdrop constant to keep in step with the first, for a card that is the menu's other face.
Which of the two is showing is still simulation state on exactly the terms the mode is, because it decides what a click at a pixel means.

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

**The window is resizable, and the picture scales with it — by the height.**
Everything here is laid out, hit-tested and simulated against the one fixed `kUiCanvas`, and **nothing inside the tick path learns what size the window is.**
`RenderSystem` builds a `gfx::ViewportRenderer` over the window's reported size and the canvas once per frame and draws every scene, every UI painter and every readout through it, so the reported size decides how big the result is blitted and where, and decides nothing else.
That is [`docs/resizable-windows.md`](../../docs/resizable-windows.md)'s sanctioned offset generalised to an offset and a uniform scale, and it is safe for the same reason: applied after every decision, applied identically to everything, never asked what a pixel means.

The height drives the scale, so a wide monitor and a narrow one of the same height draw the city equally tall and differ only in how much bar is left over; the aspect ratio is fixed and the remainder is pillarboxed or letterboxed, which is what keeps a toolbar anchored to an edge anchored to the *canvas's* edge.
Widening the canvas with the window so that a wide monitor showed more world was refused rather than overlooked — that would make a layout a function of the reported size, which is the escape that document already rejects.
`RenderSystem` paints the bars last rather than first, so a tile reaching past the canvas's edge is covered by one instead of showing in it.

**The pointer is mapped back into canvas pixels upstream of the recorder**, by `app::WindowPointerMapping` through `input::InputPipelineOptions::pointerMapping`, so what a `--record` file holds is already a canvas coordinate.
A session therefore replays identically at any window size, on any machine, with no window geometry in the file — which is the property the whole arrangement exists for, and `ViewportReplayTest` is where it is asserted end to end.

**F10 fills the screen, and it is not this app's simulation state.**
Fullscreen changes what `IWindow::size()` reports and nothing else, so with the scaling above in place it enlarges the picture and moves no hit target.
It could not live in a sink — a sink is downstream of the recorder, inside the tick path — so `main` wraps the source in an `app::FullscreenToggleSource`, a pure observer that reads the key press and calls `setFullscreen()`.
The press itself is ordinary recorded input, so a replay fills the screen where the run did and reaches the same city either way.
That is why F10 no longer opens the menu modal: the bar's `menu` button is the whole route in now, and `UiSink` reads no key at all.

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

**Where it goes next is anything but back the way it came, chosen at random among what is left — and back the way it came only when nothing is.**
That is still one rule rather than two: reversing is what remains when the set of everything else is empty, which is what a dead end *is*, so no branch in `nextFacing()` tests for one.
It used to prefer a right turn, then straight on, then left, which is deterministic, cheap and reads as a bug — every walker leaving a junction the same way makes a district's traffic run in visible circles and files a whole city's walkers round one block.

**The randomness is stateless, and that is the whole of its determinism argument.**
`wanderRoll()` seeds one `rng::SplitMix64Rng` per decision from the tick, the cell being left and the direction the walker arrived facing — three things a replay and a reloaded save both reach again — and reads one word out of it.
A generator advanced once per decision would be state living in a system rather than in the `World`, so a save would not cover it and a city reopened would roam differently from the one that was saved; that is exactly why `stepTowards()` keeps no route in a component either.
The tick is in the seed as well as the cell so that a walker coming round to the same junction facing the same way can still make a different choice of it, and two walkers meeting on one cell facing one way on one tick turn together — which is not a collision anybody has to resolve, since walkers do not collide.

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

Because a cell's box is twice as wide as it is tall, every square footprint's diamond comes out 2:1 — the same shape as one cell's, said larger.
So a square block *is* a diamond, and its art is one sprite from the sheet its footprint names — `buildingAtlasOf()` derives the sheet from `footprintOf()`, so a block and its art cannot disagree about size.
A 2×3 block is a hexagon that would need a source rect of its own, a half-tile-quantised band in a sheet, and a much heavier contract with whoever draws it.

`canPlace()` is the one statement of what a block will land on, used by `GridSink` *and* by `ghostFor()`, so what a preview promises and what a click delivers cannot drift.
A refused block is shown reddened rather than hidden, since a refusal somebody can see is one they can act on.

**The block a click would take is outlined as well as tinted**, by `footprintOutline()` — four points traced round the block's own `footprintBounds()` diamond, inside the sprite box the ghost is blitted into, so a border claims exactly the cells the click would take whatever the art's headroom does above them.
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

## The three pieces round the grid

**The furniture is laid out against the fixed `kUiCanvas` and nothing else**, so where a widget is is a function of the canvas, the strings and the simulation state — never of the window, which is what keeps a recorded click hitting the same button at any size.
`Toolbar::describe()` is one function producing all three pieces plus the city-sized gap between them, because a hit-test is a function of the layout and three layouts computed apart would agree only until one of them changed.

At the shipped 1024x640 canvas that comes out as a 1024x56 strip along the top, a 216x528 panel down the right at x=808, a 1024x56 strip along the bottom at y=584, and 808x528 of city in the middle.
Everything scales off `ui::scaleForCanvas()`, so the numbers are the canvas's rather than constants written twice.

**The gap in the middle is a `spacer`, and that is the whole mechanism.**
A spacer fills no pixels, so `ui::Interactions::pointerOverUi` is false over it, so `GridSink`'s existing "what the UI covers, it covers from the grid too" rule leaves a press there to the city with nothing invented for it.
`ToolbarTest` asserts it off `ui::Frame::rects` rather than by sweeping the canvas: every palette button's rectangle is inside the side panel's, and the corners of what is left report no widget at all.

**The palette is two columns rather than one**, and that is arithmetic rather than taste: eleven buttons in a column want more height than the middle band has, and `antwika::ui` answers a container with too little room by shrinking its children in proportion — which is a button whose caption no longer fits inside it.
Each button is a fixed width for the same kind of reason, since a column as wide as its own longest caption is not a column.

**The top strip's `game` menu is a `ui::dropdown`, and whether it is open is simulation state** in exactly the sense the camera and the selected tool are: an open list sits over the city, so whether it is open decides whether a click at a pixel chooses "load game" or lays a road.
`antwika::ui` deliberately keeps no such flag — a `DropdownSpec` carries it in and `Interactions::chosen` carries the answer out — so it lives in `UiSink`, is written inside the tick path, and is regenerated by a replay from the click that dropped it.
No `game.*` or `ui.*` event exists for opening it, closing it or choosing from it, exactly as none exists for the modal.

**There is a second dropdown beside it, and it chooses which picture of the city is showing.**
`MapView` is seven values — the city itself, plus desirability, food, water, health, fire and damage — and every one but the first paints one number about the city over the ground.
Which two of those seven a value paints is `mapViewService()` and `mapViewResource()`, two tables held apart by a `static_assert` for `serviceConferredBy()`'s reason exactly: a good is an amount and a service is a state, and a view answers one or the other and never both.
A second `static_assert` says every `Service` has a view of its own, so a service nobody can look at is a build failure rather than a countdown only the code can see.

**What each view paints has two shapes, and which one it has falls out of those tables.**
Desirability is genuinely per cell and is read straight off the field the `"serve"` phase rebuilt.
Everything else is a fact about a *building* — what is on its shelves, how much longer a service still reaches it — and is painted over every cell of that building's block, because a block is the smallest thing any of those numbers is true of.
`OverlayField` is the crossing: a percentage per cell, sparse and ordered by `Cell` exactly as `DesirabilityField` is, so turning ticks of coverage, units of food and a desirability into one scale happens in one place.

**It is painted as the ground sprite tinted rather than as rectangles**, for the reason the ghost's border is four lines: a cell is a diamond and `drawRect()` takes an upright box.
The scrim goes over *every* cell and the value over the ones that have one — a district nothing reaches is exactly what somebody opens an overlay to find, so it has to be visibly dark rather than merely unpainted.
It is painted under the walkers, since a walker is a thing in the city rather than a fact about it.

**The selection is simulation state on the pause's terms**, owned by `main.cpp` beside the camera and written by `UiSink` inside the tick path; whether the list is open is on the *game menu's* terms, since an open list over the city decides what a click at a pixel means.
Neither is persisted and neither is in `GameSummary`: a save holds a city rather than which way somebody was looking at it, and no overlay changes a single thing a run computes.
The two lists have a flag each rather than sharing one, and opening either puts the other away — though from an open list that takes two presses, since a press anywhere but the list puts it away and does nothing else at all.

**The closed box names the view that is showing** rather than saying "view", so which picture is up is readable without opening anything.
The city goes on running and taking presses underneath whichever is showing: an overlay is something to read, and nothing in one is a thing to click on.

**A press anywhere but the list puts it away, and that press does nothing else at all.**
It is reported through `UiOverlay` as the UI's even though it landed on no widget, so `GridSink` skips it: one click that both dismissed a menu and laid a tile under it would be two things nobody asked for, which is the same trap `Events.hpp` describes about persisting a click and what it caused.
Pressing the closed box again is the one exception, since that is what deliberately closing it looks like.
Only a *press* may dismiss it — nothing else activates a widget, so without that guard the `engine.tick` ending every tick would put the list away before anybody could read it.

**What an item does is `IMenuCommands`', not `UiSink`'s.**
Every one of the five is a transition another route already reaches: leaving for the main menu is the modal's own item, showing the picker is the main menu's Load Game, and putting the city away is the world-map key.
Writing them once behind that seam is what keeps the menu from being a second set of rules about the same transitions — the modal's main-menu item goes through the very same verb — and it lets what an item *does* be asserted against a real session while what a click *hits* is asserted against a layout, without either test dragging the other's collaborators in.
Save and load share one verb because they share one screen: the picker is where a session is both written out and read back, and two verbs asking for it would be one thing said twice.
`new game` restores an empty `SaveGame` through `SessionStore`, which is the one route into the live grid a load already goes through, so "empty the city" is not a second way of doing it; the other cities of a world keep what was built on them, since a session holds one live grid.

**Moving a widget changes what every recorded click means**, so `replays/demo.jsonl` was recorded again against this layout — by driving the application with `--replay` over the re-aimed session and `--record` writing what it actually dispatched, rather than by editing the file.
It reaches the identical city either way: ten road tiles, one house at (4,3) at `tent` with water coverage 474, service reach 25, camera at pan (512,48) and zoom 3 after 92 ticks.

**It houses nobody, and that is the immigration rule showing rather than a regression.**
Its ten road tiles are laid in the middle of the grid and none of them reaches an edge, so there is no gate for anybody to walk in through — see the population section below.
The file is still what pins the layout and the determinism, which is what it is for; a session that wanted people in it would have to lay road out to the edge first.
`BootstrapTest` pins the two pixels that file depends on — the main menu's New Game and the palette's House — so a layout change fails a test rather than being rediscovered by hand.

**The menu modal is a modal rather than a mode, and whether it is up is simulation state.**
The toolbar's `menu` button opens it, `UiSink` owns the flag, and no `ui.*` or `game.*` event exists for any of it — a recording holds the click, and a replay works out again which widget it hit.
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
`PauseState::set(bool)` is idempotent, so a value arriving twice — a replay redelivering a press, or a press against a bar that is already showing what it asks for — settles on one answer instead of undoing itself.
What the button sends is still worked out from the state it read, though, and `UiSink` re-describes the bar between the events of one tick, so two pause presses landing inside the same tick *do* cancel: the second one reads what the first one set and asks for the opposite.
Making them agree would mean carrying the intended value on distinct pause and resume widget ids, and a city one player builds has not needed it.

**A save is version 2 and carries the buildings.**
Kinds, stock, risk and all three countdowns — countdowns reset on load are exactly the lockstep they exist to avoid.

The building/walker link is persisted **as a pair of array indices rather than as an `ecs::Entity`**, because `EntityManager` hands ids out from a monotonic counter and a restore destroys and recreates everything, so a raw handle would name nothing on the way back in.
Reading refuses an index past the end of the array it points into, and refuses a pair that disagree about each other, rather than repairing either — a repaired save is a session somebody never had.

Restoring creates every entity *before* adding any component, because `create()` is immediate where `add()` is staged — so a link has to be built into the component rather than written onto it afterwards.

**Walkers do not collide.**
Two may occupy one cell, because nothing requires otherwise and a rule to avoid it would be a requirement nobody asked for.

**The atlases are hand-drawn art, and `TileAtlas.hpp` is their address map.**
There are three sheets under `assets/` — `atlas_1x1.png` for the ground, the roads, the walkers and the 1x1 buildings, and `atlas_2x2.png` and `atlas_3x3.png` for the bigger blocks — drawn and curated directly, so nothing rebuilds them and editing one is editing the art; see [`game-texture-atlas.md`](game-texture-atlas.md), which is the contract with whoever draws them.
The header says where a sprite lives arithmetically rather than as a table of rectangles, so repainting a sprite is free and moving one is not.
What is left to catch a mistake is that header's `static_assert`s, `TileAtlasTest`, `SpriteBoundsTest`, and a check at startup that each PNG really is its expected size.

**A sprite is taller than the diamond it stands on, and `SpriteBounds.hpp` is where that is resolved.**
Each sheet states a pivot — the bottom corner of the sprite's footprint diamond — and blitting anchors that point to the block's own bottom corner on screen, so headroom rises above the cell and the base block's skirt hangs below it.
The consequence the scene owns is paint order: a skirt must sit under the cells south and east of it, so `GridScene` paints terrain and buildings in one pass, a diagonal of cells at a time, laying each building with the diagonal its block starts on and no grass at all under a standing building.
The walkers still come last, so a walker is never hidden by what it is standing on.

Which of the sixteen road sprites a junction shows is worked out in `GridScene` by binary-searching the snapshot's ascending paths, and stays out of `SceneSnapshot` and `GameSummary` — it is a picture, not state a replay has to reproduce.
The sheet orders its junctions by arm count rather than by link mask, so `kRoadSpriteByLinks` in `TileAtlas.hpp` is the one crossing between the two orders.

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

**A household's tier and how full its house is are listed on one `housesPeople()` test**, because both are facts about a household and a well has none — a well is on `HousingLevel::Tent` and houses nobody, and saying either about it would be saying something untrue.
The occupancy reads `people 3/10`, against `populationCapacityOf()` rather than as a bare count: whether a house has room left is what decides whether the district still grows, and `people 3` is a number a reader can do nothing with until they know what a shack takes.

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

## Service coverage, desirability, and why risk stopped being a subtraction

**A service is a state with a lifetime, and a good is an amount.**
That distinction is the whole of this section.
A market seller hands food over and is a hundred units poorer for it; a water carrier walking past a house makes that house watered and the well is no poorer at all.
Modelling water as a good meant a delivery per house per drain; modelling it as coverage means a walker has to keep *reaching* the district, which is the thing the road network is actually for.

`Coverage` is one countdown per `Service`, in ticks, on a component of its own.
`CoverageSystem` decays every countdown by one each tick and then tops up to `kCoverageFull` for every service a walker standing beside a building confers.
The decay runs first so a walker leaves a full countdown rather than one tick short of one.

**Counting in ticks left rather than in a percentage is what removes the second countdown.**
Every other period in this application carries a per-building countdown precisely so two buildings put up a tick apart do not fall into lockstep for ever.
A coverage that decayed on its own period would have needed one too; a coverage that *is* a countdown decays by subtracting one and needs nothing.

**The component is optional, and an absent one means uncovered rather than unknown.**
`coverageOf()` answers zero for a building that has none, which is what lets a housing rule, a rating or a hover panel be written against this before any well exists -- and it is exactly what a version-3 save written before coverage existed says, so the format grew one optional `"coverage"` array and no migration.
`setCoverage()` is the one writer, because "add a component or set an existing one" is a decision rather than a detail: `World::add()` is staged and `World::set()` refuses an entity that has none yet, and asking once is what keeps a building from acquiring an all-zero component just to simplify a call site.
A building is given one the first time somebody reaches it and never before.

**Risk moved onto coverage but stayed in `BuildingSystem`.**
A fireman used to subtract `kRiskRelief` from whatever he walked past, which was coverage said as a subtraction: it made "somebody came recently" the only thing that mattered but expressed it as an amount.
`BuildingSystem::age()` now asks `coverageOf()` which way a building's risk steps -- up where `Service::Safety` or `Service::Structure` has lapsed, down where a walker is keeping both alive -- and the fire station and the engineer's post stop being arms in the delivery code.
Putting the *step* in `CoverageSystem` was tried and rejected: `risk` is a field of `Building`, `age()` is the one place a building's countdowns advance and the one place `isLost()` reads the amounts this tick produced, and splitting it would have meant two systems in two phases writing one component with a tick between the risk that finished a building and the pass that noticed.
The coverage `age()` reads is therefore the previous tick's, which is one tick out of five hundred on a countdown whose only job is to say whether somebody came recently.

**Desirability is a sum, and that is its whole determinism argument.**
`DesirabilityField` is rebuilt every tick from the buildings by `DesirabilitySystem`, as a linear integer falloff over Chebyshev distance from each block.
Integer addition is commutative and associative, so the field is a pure function of the *set* of buildings rather than of the order `ecs::View` happened to walk them in -- which is what lets it read a view whose order is "whichever storage has the fewest entities" and nobody's to name.
`DesirabilityTest` builds the same city twice in opposite orders and asserts an identical field, and that assertion is the point of the type.
It is rebuilt rather than edited because a field kept up to date by adding a contribution when a building goes up would have to be told about a demolition by risk, a demolition by starvation, a city switch and a save restore, and every one it was not told about would be a district that stayed pleasant because something that burned down was still counted.

**No `game.coverage_lapsed` event, and this is the tempting one.**
A notification would let a panel react without polling.
It is rejected for the reason [`Events.hpp`](../../src/apps/game/include/antwika/game/Events.hpp) gives at length: coverage is a function of walkers, which are a function of buildings, which are a function of clicks, so it is regenerable at every step -- and a notification is a picture rather than an input.

`BuildingView` carries the coverage and `GameSummary` therefore compares it, because whether a district is served decides whether it gains risk and, later, whether it grows; a run and its replay disagreeing about that is a divergence, and the summary is where a divergence is caught.
`BuildingSprite` carries a copy so the hover panel can list it, and the panel lists a service only where it is above zero -- risk is a fact about any building, so the question applies to every kind, and an absent line and a line reading nothing say the same thing.

## The goods chain, and why a market has a buyer of its own

A farm grows food, a cart pusher hauls a load of it to a storehouse, a market sends a buyer to fetch from that storehouse and a seller to hand it to the houses it walks past.
That is raw material -> store -> market -> consumption, end to end, and it is the first time a walker in this application has a *destination* rather than a preference order.

**`Errand` is what makes a walker routed, and it is one component rather than two.**
A walker carrying one is steered by `stepTowards()`; a walker without one roams by `nextFacing()` exactly as every walker did before the component existed, which is why `WalkerSystem` gained one arm and changed nothing else.
Its destination may be `kNullEntity`, and that is an ordinary state rather than a missing one: a cart loaded in a city with nowhere to unload takes the load round with it and hands it to the houses it passes, which is precisely what the food walker of the version-2 vocabulary did.
That is not a fallback bolted on afterwards -- it is what keeps a city migrated from a version-2 save fed while it has no storehouse in it yet, and `SupplyChainTest` asserts both cities: the full chain, and a farm standing alone.

**A cart may only unload into a kind that sends nobody, and the reason is the walk phase rather than the fiction.**
A load changes hands in `BuildingSystem`, which shares the `"walk"` phase with `SpawnSystem`.
Both read a building as of the last commit and both write the whole component back, and the cadence's write is the later of the two -- so a delivery into any kind that sends walkers is undone in the same tick it was made, silently, whenever that building happens to have nobody out.
A storehouse is the one kind that sends nobody, which is what makes it the one kind `acceptsAt()` names.
Everything else that receives goods is credited by the system that owns it, in a later phase where nothing else writes it: that is why a market has a buyer of its own instead of being carted to, and why a workshop has one too.

**Clay becomes pottery because a workshop goes and fetches it, and that is one code path rather than a second chain.**
`fetchedFromStores()` is the table of kinds that send a buyer out for something they cannot make -- a market for the food its seller hands out, a workshop for the clay it fires -- and `SupplySystem` (which is what `MarketSystem` grew into) drives both off it.
A `static_assert` holds it to `consumedToProduce()` wherever both have an answer, so a kind that eats an input and has no way to get one is a build failure rather than a building that stands for ever with an empty hopper.
The whole chain closes on itself: a clay pit digs, a cart pusher hauls to the storehouse, the workshop's buyer fetches it back, `ProductionSystem` fires it, and the workshop's own cart pusher takes the pottery to the storehouse like any other output.

**That system has a phase of its own now, `"supply"`, and a workshop is the reason.**
It used to share `"haul"` with `HaulingSystem`, which was safe on exactly the grounds stated above -- a cart is loaded out of a producer, a buyer is loaded out of a storehouse, and no building is both.
A workshop is both: pottery out, clay in.
So the two would read one `Building` as of the same commit and write the whole component back, and the later write would silently undo the earlier -- which is the trap the phase list exists to avoid, met for the second time.

**The chain runs in two phases, `"produce"` then `"haul"`, rather than the one the plan sketched.**
A phase is where the World's buffers swap, so two systems in one phase both read what the last swap left and both write a whole `Building` back.
Adding a batch to a farm and taking a cart-load off it is therefore not arithmetic while they share a phase -- the later write silently undoes the earlier -- and the commit between them is what makes it arithmetic.
`HaulingSystem` and `MarketSystem` do share a phase, because nothing they write overlaps: a cart is loaded out of a producer and a buyer is loaded out of a storehouse, and no building is both.

**Every new decision that splits a limited amount is walked out of a `std::map` rather than a view.**
`ecs::View` iterates whichever storage has the fewest entities, which is reproducible for a given history and is not an order anybody can name.
Producers are walked in ascending `Cell` because a producer's stock is split among the carts it has out; markets are walked in ascending `Cell` because two of them buying from one storehouse split what it holds; and `BuildingSystem`'s walkers are walked in ascending `(Cell, Entity)` because two of them filling one shelf split what room it has and `deliverTo()` clamps.
`BuildingIndex` keeps two buildings off one origin cell, so a cell alone is already total for the first two; the entity is in the third key because walkers may share a cell.
`nearestAccepting()` orders by path length and then by ascending `Cell` of the store, and its `GridGraph` is built over the configured `GridExtent` for the reason `Homing.cpp` gives -- a bounding box of the roads would renumber every node as one was laid, and the tie-break with it.

**No new event kind, and the tempting one was `game.goods_delivered`.**
It is derived from a walker's position, which is derived from a route, which is derived from a click, so a recorder would write it beside the click and a replay would deliver twice.
`Events.hpp` already states that rule about placing a tile and this is the same rule.

**Nothing here bumped the save format.**
`SavedBuilding` gained an optional `"ticksUntilOutput"` and `SavedWalker` an optional `"errand"`, and absent means what a version-3 file held: no batch under way and no errand.
The errand's destination is an index into the buildings array rather than the `ecs::Entity` it is in memory, because a restore destroys and recreates every entity -- and it is refused when it points past the end of that array rather than repaired, exactly as `home` is.
`CityGrid` carries both across a city switch for the reason it carries every countdown: a city reopened with them reset is a city whose producers finish in lockstep from then on, and a loaded cart that lost its errand would be a cart holding goods nobody can name.

## Housing evolution, and the ladder that can only demand what a seller carries

**A house has a tier, and the tier is what the rest of the city is arranged to raise.**
`HousingLevel` is `Tent`, `Shack`, `Hovel` and `Cottage` — four, which is the fewest that makes both "grows" and "shrinks" ordinary rather than each being the edge of the ladder.
`kHousingRequirements` says what each tier demands: a desirability at the house's own cell, a flag per `Service` that must still be reaching it, an amount per `Resource` on its shelves, and the population it houses when full.
`HousingSystem` counts a house that holds the *next* tier's row toward `kEvolvePeriodTicks`, and one that has stopped holding *its own* row toward `kDevolvePeriodTicks`.

**The bottom row demands nothing, and that is load-bearing rather than decorative.**
A house on bare ground meets `Tent` by construction, so there is no rule anywhere saying it cannot devolve below the bottom — it simply never fails the row it is standing on.
The top tier is handled by the ladder running out rather than by a check of its own, for the same reason.

**Every demand rises with the level, and a `static_assert` holds the table to it.**
That is what makes the two arms mutually exclusive: meeting the next row implies meeting this one, so a house is never owed a promotion and a demotion in the same tick, and `HousingSystem` depends on that rather than checking for it.

**The ladder may only demand food, and the `static_assert` saying so is the most useful line in the header.**
A market seller is the one walker that ever hands goods to a house, and `carriedResource()` says it sets out with exactly one resource.
Clay reaches a storehouse and a workshop and pottery comes back out of one, but a house sees neither: a market seller is still the one walker that hands anything to a house, and food is still what it sets out with.
A tier demanding either would have been a rung no city could ever stand on, and nothing would have said so; the assertion turns that into a build failure the day somebody adds a cart that changes the answer.
The desirability figures are read off `kDesirabilityOf` and its integer falloff rather than guessed: a well truncates to nothing one cell away, so a threshold of 1 means a market or a doctor within two cells and 2 means both.

**Coverage is asked whether it reaches at all, never how much of it is left.**
A coverage countdown's whole job is to say whether somebody came recently, and a threshold on it would be a second, unstated period sitting on top of `kCoverageFull`.

**Evolving is per house, out of that house's own state, so no order over entities exists to get wrong.**
Nothing here splits a limited amount and nothing one house does changes what another is owed, which is what lets `HousingSystem` read `ecs::View` directly.
**There is deliberately no merging of 1x1 houses into blocks**: it is the one housing rule that would need a total order over *neighbours* rather than over one entity's own fields, and it would have doubled the work for a picture.

`Household` is an optional component holding the level, the two countdowns and the occupancy, and an absent one means the bottom tier, a fresh countdown each way and nobody living there.
A house is given one the first time it has something to say and never before, exactly as a building is given a `Coverage` the first time somebody reaches it — a default component and no component are one thing, and writing one anyway would put a member in every save file for every house that has never done anything.
**The occupancy is stored here and moved by nothing in this increment**: a tier's capacity and a house's occupancy are one fact, so the number belongs beside the tier, and the rules that raise and lower it are a later workstream's.
`HousingQuery.hpp` is the read-only face of all of it, and every answer is total — `levelOf()` on a well, on a road, or on a handle whose entity is long dead is `Tent` rather than a throw.
It says `populationCapacityOf()` rather than the `capacityOf()` the plan sketched, because `Store.hpp` already answers `capacityOf(BuildingKind)` about how much of a *good* a building holds, and two overloads of one name meaning two different capacities is an ambiguity a reader would have to resolve by looking at the argument's type.

**`"settle"` is a phase of its own, after `"haul"` and before `"observe"`.**
A phase is where the World's buffers swap, so a house is judged on the shelves this tick's sellers filled rather than the previous tick's, and on the desirability field the `"serve"` phase rebuilt two phases earlier.
`HousingSystem` writes `Household` and nothing else — it never touches `Building` at all — which is what lets a later workstream put its own systems beside it in that phase.

**No `game.house_evolved` event, and this is the tempting one.**
It reads like a notification worth recording.
It is a pure function of coverage, stock and desirability, every one of which a replay regenerates from the clicks that built the district, so a recorder would write it beside those clicks and a replay would grow the house twice.

**Nothing here bumped the save format either.**
`SavedBuilding` gained one optional `"household"` object, and absent means the bottom tier, a fresh countdown each way and nobody living there — which is exactly what a version-3 file written before housing said.
It is one object with all four members required rather than four optional members side by side, because the four only ever mean anything together: a record naming a tier with no countdown beside it is a house half-written, and the validator refuses one rather than reading three fields and guessing the fourth.
The countdowns are persisted rather than reset, for the reason `Building`'s three are — two houses reopened with the same number grow and shrink in lockstep from then on, which is precisely the lockstep a per-building countdown exists to avoid.
`CityGrid` carries the household across a city switch on the same terms, and through `setHousehold()` rather than `World::add` directly, exactly as it carries coverage.

`BuildingView` carries the level and `GameSummary` therefore compares it, so a run and its replay disagreeing about a house growing fails `ReplayDeterminismTest` directly; `BuildingSprite` carries a copy so the hover panel can name the tier, and it names it rather than numbering it, since "level: 2" is a number a reader has to look up.

**`BuildingView` carries the occupancy on the same terms, and that made the replay comparison strictly stronger.**
Before it, a house's people reached `GameSummary` only as `CityRatings::population`, a sum over the whole city — and a sum hides a swap: two houses trading one occupant total the same, so a live run and its replay could disagree about *where* the city's people were and still compare equal.
Per house they cannot, which is why the number went onto the view rather than only onto the sprite that draws it.
That is the opposite call from stock, deliberately, and for the reason coverage and the level are: what a building is holding is a gauge, and who lives in it is what the whole city is arranged to raise.
`HousingSystemTest` is a loop over `kHousingRequirements` rather than four hand-written cities — a row added to that table is a row it already covers, which is the whole reason the requirements are a table and not a switch.

## Population, labour and the first thing that judges the city

**People are the resource everything else competes for, and that is the whole of this workstream.**
A house with a road beside it, standing on ground its tier finds acceptable and with room left, takes one person in every `kSettlerPeriodTicks`; one below its tier's threshold, or holding more than its tier can, loses one on the same countdown and stops at nobody.
The sum of everybody in the city is a workforce, every workplace says through `workersWantedBy()` how many of them it wants, and what it actually gets decides how fast it works.

**A workplace nobody works at does nothing at all, and one half-staffed takes twice as long.**
`workedPeriod()` is the one place staffing becomes a rate: it hands back the period at a full complement, the period stretched by `wanted / filled` below one, and *nothing at all* when nobody turned up.
Nothing rather than a very large number, so a caller has to say what it does about it — `SpawnSystem` and `ProductionSystem` both hold their countdown where it was rather than resetting it, which is the rule they already followed for a building at the walker cap and a workshop out of clay.
A building unstaffed for a thousand ticks therefore owes nobody a thousand walkers the moment somebody arrives.

**Labour allocation is this increment's one genuinely contended decision, and it is walked out of a `std::map` keyed by the workplace's origin `Cell`.**
`ecs::View` iterates whichever storage has the fewest entities, which is reproducible for a given history — so a replay of one run agrees with it — and is not an order anybody can name, and it *moves* as component counts cross each other.
That is fine for a loop whose body is independent per entity and it is not fine for splitting a limited amount, which is exactly what this is.
**No tie-break is needed at all**, because `BuildingIndex` refuses a second building on an occupied cell: two workplaces cannot share an origin, so the key is unique by construction rather than by a rule written beside it.

**`kWalkerLimit` is the third limited amount, and `SpawnSystem` splits it the same way.**
Sixty-four walkers between however many buildings want to send one is exactly the shape labour has, and the system used to hand the last free slots to whichever buildings a view happened to visit first.
A live run and its own replay still agreed -- a view's order is reproducible for a given history -- so only *restore-order invariance* broke, and only at the cap: one save loaded twice with its buildings in two different array orders could disagree about which building sent the last walker.
It now collects its senders into a `std::map<Cell, Entity>` and walks that, with no tie-break needed for `BuildingIndex`'s reason, exactly as labour and the markets do.

**`AllocationOrderTest` is what would catch a regression there, and nothing in the tree before it would have.**
It restores one city twice through `SessionStore`, with the buildings in two different array orders, runs both for the same number of ticks and compares what they came to.
Restoring rather than clicking is deliberate: `restoreCityGrid()` creates entities in the order the array holds them, so the array order is exactly the variable being changed — clicking the same city together would move the tick each building was placed on as well, and every countdown with it.
The one member it sorts before comparing is `GameSummary::buildings`, which lists what is standing in the world's own order and *is* therefore the creation order; sorting it turns the comparison into "the same things, wherever they were listed", which is the claim being made.
It runs a second city for the walker cap: ninety-six wells, each one cell with a road beside it and a saved `ticksUntilSpawn` of nothing, so every one of them asks on the very first tick and a third of them have to be turned down.
The city is asserted to be genuinely at the cap first, for the reason the first one is asserted to be genuinely short of people: two runs that both sent everybody would agree for the wrong reason.

**Nobody appears out of thin air any more, and nobody vanishes into it.**
A house that is due somebody sends for them, and what arrives is an ordinary walker carrying a `Journey`: it enters at the nearest road on the *edge of the map*, walks the road network to the door, and the house's number goes up on the tick it gets there rather than on the tick it was sent for.
So a district a long way from a gate fills more slowly than one beside it, and **a city no road reaches out of takes nobody in at all** — which is a fact about the road network rather than a rule written anywhere.

A house that sheds somebody sends one out the same way, and where they go is decided in that order: `nearestVacancy()` for the nearest house with a bed going, and failing that `nearestGate()` for the nearest road out of town.
So a house that has just devolved does not evaporate its overflow — the people it can no longer hold walk to whatever room there is, and the city's total only falls when there is none.

**A road on the edge of the extent is the whole of what a city border is here.**
There is nothing beyond the extent, so a road that reaches it is a road that leads somewhere else; `nearestGate()` orders the candidates by route length and then by ascending `Cell`, which is `nearestAccepting()`'s order and is total for the same reason it has to be — which gate a migrant uses decides which roads they walk down, and a replay has to pick the same one.

**A house asks for nobody while somebody is on the way**, and the bookkeeping for that is the handle in its own walker slot rather than a flag: a house already knows which walkers it has out, and `ecs::EntityManager` never reusing an index is already why a stale handle can only be dead.
Somebody walking *out* takes no slot, because the house they left is not waiting for them.

**Sending is still per house and takes from nothing anybody else takes from**, so it reads `ecs::View` directly; there is still no shared migrant pool.
*Arriving* is not, because two people reaching one house on one tick split the room it has left, so the arrivals are walked out of a `std::map` keyed by ascending cell and entity — the entity being in the key because two walkers may share a cell.
They are counted before a single household is written, and folded into the one write each house gets, because `HousingSystem`'s trap applies to a system racing *itself* just as much as to two of them.

`Journey` is a component of its own rather than a member on `Errand`, and the two are never both on one walker.
An errand names a building and says what is in the cart; a journey names either a house somebody is moving into or a way off the map they are taking, and there is nothing in the cart at all.
Putting a cell on `Errand` was tried and rejected: `errandTarget()` answers with an `ecs::Entity`, every reader treats `kNullEntity` as "not routed", and a load bound for nowhere already means something else entirely.

**`PopulationSystem` runs in a `"populate"` phase of its own, after `"settle"`, and this is the trap the phase list exists to avoid.**
`HousingSystem` writes a whole `Household` back and so does this; two systems in one phase both read what the last buffer swap left, so the tier one wrote and the occupancy the other wrote could not both survive the tick they were written in — the later write would silently undo the earlier, some of the time, and a divergent replay a long way from its cause is what that looks like.
Two positions in one phase do not make two systems sequential; a commit between two phases does.
`LabourSystem` shares the new phase on its own terms: it writes `Workforce`, which nothing else writes, and it reads the population as the settle phase left it — so a person who arrived this tick is employable from the next one, which is a lag nothing can see.

**`Workforce` stores what was allocated and never what was wanted.**
How many workers a kind wants is `workersWantedBy()`, a table, for the reason `footprintOf()` and `kDesirabilityOf` are tables: a copy on the component could disagree with the kind standing on the cell, and a save that disagreed with itself is a session somebody never had.
An absent `Workforce` means **fully staffed**, which is the rule this whole increment is written under — an absent component is the value the game had before the component existed — and it is what let the goods chain be built and pass before there were any people to allocate.
A house wants nobody because nobody works where they live; a storehouse wants nobody because nothing in this round reads its staffing, so a demand there would take people off buildings that do something with them with no effect a player could see to explain where they went.
A `static_assert` holds "wants workers" and "sends somebody out" to being the same list.

**`CityRatings` is the first thing in this application that judges the city rather than simulating it.**
Four integers — population, the share of the city's jobs that are staffed, the mean housing tier in hundredths, and the share of house-and-service pairs a service still reaches — and `ratingsOf()` is a pure function of the `World`.
Every member is an integer because the ratings are compared in `GameSummary` and a float from a division does not have to round the same way on two toolchains.
Nothing is persisted, because every member is a sum over what a save already holds.
The plan for this increment also handed `ratingsOf()` the `DesirabilityField`; it does not take one, because not one of the four members is a function of it and an argument a reader has to check the body for is worse than one that is not offered.

**`GameSummary` carries the ratings, and that is what makes a divergence in the city's people fail `ReplayDeterminismTest` directly.**
A house's occupancy is not in `BuildingView` and a workplace's share of the workforce is not either, so without this a live run and its replay could disagree about both and still compare equal.
`ReplayDeterminismTest` gained a session that is genuinely short of people — two farms wanting eight between them and one tent holding five — because a determinism test over a city that contended over nothing agrees for the wrong reason.

**The two ratings labels sit at the far end of the bottom strip**, after a growing spacer, so a strip that gains a button does not move them along.
They are labels rather than buttons, and not by omission: there is nothing to press, so they declare no `WidgetId`, and a rating can never become an input.
`RatingsSystem` keeps the value the bar reads, in the `"observe"` phase ahead of the observers — the one place it sees the tick it is reporting on rather than the one before — and it recomputes the whole answer for `DesirabilitySystem`'s reason, since a running total would have to be told about a demolition, a city switch and a save restore, and every one it was not told about would be a rating flattering a city that no longer existed.

**No new event kind, and this is the workstream where one was most tempting.**
`game.immigrant_arrived` is a pure function of a road, a field and a tier, all of which a replay regenerates from the clicks that built them, so a recorder would write it beside those clicks and a replay would house the same person twice.
That holds just as well now the arrival is a walker reaching a cell: where a walker is is a function of a route, which is a function of a click.
`game.set_wage` is worse: a wage is a click on a widget resolved by a sink inside the tick path, which is exactly what "no `ui.*` event name may ever exist" means.

**Nothing here bumped the save format either.**
`SavedWalker` gained an optional `"journey"` object, absent for every walker that is not a person on the move — which is what every walker in a file written before this was — and its house is an index into the buildings array refused when it points past the end, exactly as an errand's destination is.
`SavedBuilding` gained an optional `"employed"`, and absent means nobody has been allocated there — which is both what a version-3 file written before labour says and what a workplace put up this tick holds.
The population rides on the household object housing already wrote, which gained one further optional member, `"ticksUntilSettler"`.
That one member is the only one of the five the validator does not require, and it is not an inconsistency: the other four arrived together and only mean anything together, while a file written between the two workstreams is one whose houses had no settler countdown to name, and refusing it would be tightening the schema rather than growing it.
Neither the workforce total nor any rating is written, because both are sums over what the file already holds.
`CityGrid` carries the `Workforce` across a city switch and `SessionStore` threads it through a restore, on the terms every other component is carried: an absent one means fully staffed, so a city reopened having lost one is a city that quietly speeds up for a tick.

## Key bindings, and why the layout travels in the recording

**A rebindable key is the first thing this application has that a run needs and cannot work out for itself, and that is the whole of the design.**

`antwika::sudoku`'s `KeyMapping.hpp` states the position everything else here follows: which key does what is *a layout written down rather than asked of a window system*, so what a recording holds is the symbolic key and the meaning comes back identically under any backend on any keyboard.
A player-configurable binding breaks that outright.
A session recorded where `K` zooms and replayed where `K` pauses resolves one recorded press to two different actions and diverges — silently, and a long way from its cause, which is the failure class the cross-module rules exist to prevent.

There were three ways out, and this is the one taken.

**The binding is part of the session, and it is persisted because it is externally-supplied input.**
The rule is "only externally-supplied input is persisted, and anything a sink or a system derives from it is regenerated".
A binding read off the player's own `options.json` is supplied from outside the run and is derivable from nothing inside it — no amount of replaying the clicks recovers it — so that rule says outright that it has to be recorded, exactly as `SaveGame::seed` is the thing a resumed session cannot regenerate for itself.
So it enters a run the way every other externally-supplied input enters one: through an `ITickEventSource`, upstream of `event::TickEventRecorder`, as `game.bind_key`.
`BindingSource` announces it once, ahead of the first tick's own events, and its "announce or not" is the same seam `input::InputPipelineOptions::readsDevice` already is — **a live run reads the device and the file, and a replay reads neither**.
`BindingReplayTest` is where that is asserted end to end: one session, recorded on a machine binding zoom to `K` and pause to `J`, replayed on one binding them to `M` and `L`, reaching the same city; and the same file with the announcements stripped out reaching a *different* one, so the first cannot pass for the wrong reason.

The two rejected alternatives are worth stating.
**Recording the action rather than the key** — putting the recorder downstream of the mapping — is cheaper and argues that an action is externally-supplied input, which it is not: an action is what a *layout* makes of a key, and a layout is state the run holds.
It would also have put the first derived value into a recording, which is the thing `Events.hpp` refuses at length.
**Binding only actions no recording can contain** is airtight and is not the feature: every action worth a key here changes what the run computes.

Three consequences follow, and each is load-bearing.

**Only a binding that differs from `kDefaultBindings` is announced**, so a run on a machine nobody has rebound records exactly what it recorded before any of this existed, and every replay written before it still means what it meant.

**A replay writes nothing back**, because `machineOptionsFor()` hands it neither the layout nor the path.
Replaying somebody else's session would otherwise leave the machine carrying that session's bindings, which is a side effect nobody asked for; a live run does leave its final layout in the file, where the next one finds it.

**A rebinding made on the options screen is not an event**, and must not become one.
That one is a key press resolved against the layout inside the tick path, downstream of the recorder, and a replay works it out again exactly as it works out which tile a click laid — recording both would apply it twice.
So the same state has two writers on purpose: `BindingSink` for what came from outside the run, and `MainMenuSink` for what the player did inside it.

**`KeyBindings` is a plain value with exactly one key per action**, and it is in `GameSummary` — so a live run and its replay disagreeing about a binding fails `ReplayDeterminismTest` directly, rather than only showing up on the next key nobody happened to press.
It is deliberately not `input::ActionMap`: that one holds many bindings per action and only ever gains them, so an action cannot be *re*bound, which is the whole of what an options screen does, and it is keyed by a string and is not comparable, so it could be neither a constant fixed in source nor a member of a summary.

**Binding is a total function that answers rather than throwing**, since it runs while a frame is being described.
A key another action holds is **refused** rather than stolen — stealing would leave that action with no key, which would make every action's key a `std::optional` for the sake of one gesture; two rebindings say the same thing in the order the player chose.
Escape and F10 are **reserved**, because this application spends both above the tick loop and a binding on either would fire *and* quit or fill the screen.
A refusal leaves the row still waiting, so the next key answers the same question, and the line under the rows says which of the four things happened.

**The options file states its version and is read `parse -> read version -> migrate -> validate -> decode`**, through an injected `replay::MigrationChain` like every other persisted document here.
The chain is empty — there has only ever been one revision — and it is present anyway, because it is what refuses a file from a newer build instead of decoding it on the strength of happening to satisfy today's schema.
**A missing file is an ordinary first run**, not an error: a player who has never opened the screen is playing what the build ships.
Anything else wrong with it is refused rather than repaired, on `SaveGame`'s terms — including a document binding two actions to one key or binding a reserved one, since a layout nobody could have chosen is not one to guess the intent of.

**The `Options` button was declared beside `Quit` rather than under it**, on a row of two.
The menu's card is centred in the canvas, so a fifth row would have moved all four items already there and left every session recorded before this one opening something else — the same reason the ratings labels were appended to the toolbar's first row rather than inserted among the buttons.

## Future work

**`UiSink`/`UiOverlay`/`Toolbar` should adopt `ui::applyHover()` next.**
The app already owns a hint channel and already draws its placement ghost from it, so the toolbar buttons lighting up on approach is `main.cpp` handing `RenderSystem` the channel and one `applyHover()` call after the sink has resolved the press — and it is the one remaining thing `apps/game` says it does not do.

## See also

- [`blog/013-the-camera-is-simulation-state.md`](../../blog/013-the-camera-is-simulation-state.md)
- [`blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md`](../../blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md)
- [`game-texture-atlas.md`](game-texture-atlas.md) — what an artist has to produce.
