# Caesar round 1: an implementation plan

`apps/game` is to iterate toward the Impressions Games lineage -- a walker-driven city where buildings act only through the people they send out.
This document plans the **first increment** only.
It is deleted once the work it describes has shipped, as `CLAUDE.md` requires of a plan document.

## 1. Assessment: what `apps/game` is today

### The composition root

`src/apps/game/src/main.cpp` is branchless and excluded from the coverage report.
It builds the backends, the window at `kUiCanvas`, reads and size-checks `assets/atlas.png`, constructs the state that must outlive `bootstrap()` -- `Camera`, `PathIndex`, `BuildingIndex`, `AppModeState`, `PauseState`, `RoadDrag`, three `UiOverlay`s, `WorldMapState`, `FrameMeter` -- and hands it all to `antwika::game::bootstrap(GameConfig)` in `src/Game.cpp`.
Constants that matter: `kExtent{24, 24}`, `kInitialPan{512, 48}`, `kTickInterval` 40 ms, `kFramesPerTick` 4, `kWorld{24, 16, seed = 7}`.

`bootstrap()` is where every collaborator is wired and where the tick loop runs.
It returns a `GameSummary`, which is the comparable value a replay is asserted equal on.

### The tick path

`simulation::EngineLoop` is the one code path.
`event::TickedEventDispatcher` fans each tick's events to an ordered `std::vector<std::reference_wrapper<ITickEventSink>>`, and **that order is load-bearing**:

1. `InputFold` -- decodes and holds this tick's input event; the only thing that clears an edge.
2. `AppModeState` -- commits a staged mode change at the tick boundary.
3. `GameStateReducer` -- folds `engine.tick` and `game.score_increment`.
4. `MainMenuSink`, `SaveLoadSink` -- own whole screens and gate themselves.
5. `ModeGatedSink(UiSink, CityMap)` -- resolves a press against the toolbar and the menu modal first.
6. `WorldMapSink` -- a press that opens a city must not also build in it.
7. `ModeGatedSink(GridSink, CityMap)` -- turns input into placements, then runs the scheduler on `engine.tick`.
8. `engine::StopSignal`, then the optional `replay::TickEventRecorder`.

### The reducer

`GameState` is two `std::uint64_t`s: `ticksProcessed` and `score`.
`GameStateReducer` folds exactly two names.
Everything else in the application is ECS state or a small shared object outside the `World`.

### The event vocabulary

`include/antwika/game/Events.hpp` declares **one** name, `game.score_increment`, and then spends most of the file explaining why there is no event for placing anything.
That file is normative and this increment does not weaken it.

### The ECS and the scheduler

`ecs::World` with `ecs::SystemScheduler`, two phases created in `Game.cpp`:

- `"walk"` -- `WalkerSystem`, then `BuildingSystem`, then `SpawnSystem`, each wrapped `SessionGatedSystem` (does a session exist) then `PauseGatedSystem` (has a player asked for a pause).
- `"observe"` -- the caller's observers, in practice `RenderSystem`.

The three systems share one phase deliberately.
`SpawnSystem` depends on `destroy()` retiring only at `commit()`, so a walker that died this tick still reads alive and its building does not immediately re-let.
`BuildingSystem` depends on running after the walk so a delivery sees this tick's cells.
**Nothing in this increment may re-order or split that phase.**

Components: `Cell`, `Path`, `Walker`, `Building`.
`ecs::View` iterates "whichever storage has the fewest entities", which is reproducible for a given history but is **not** an order anybody can name -- see section 2.

### Indices and shared state outside the `World`

`PathIndex` and `BuildingIndex` are ordered `std::set<Cell>` lookups that answer "as of right now" rather than "as of the last commit", which is why two clicks in one tick cannot double-place.
`WorldMapState` holds one of each **per city**, while `Building` entities stay global.
`CityGrid.hpp` / `restoreCityGrid()` is the one place entities are put onto a live grid, shared by a city switch and by a save restore, because `create()` is immediate where `add()` is staged.

### How a tile is placed

`input.pointer_down` at a pixel -> `InputFold` -> `UiSink` (may claim it) -> `GridSink::act()` -> `screenToCell(asPoint(pos), camera)` -> `place(cell, overlay.tool())` -> `buildingKindOf(tool)` -> `canPlace(origin, footprint, extent, paths, built)` in `Placement.cpp` -> `world.create()`, `world.add<Cell>`, `world.add<Building>`, `built.insert()`.
A road is `canPave()` plus `paths.insert()`, and a left-drag runs `planRoad()` -- an A* through `antwika::pathfinding` over the **configured** extent, never a bounding box of the roads.

### What the economy currently does

`Resource{Food, Water}`.
`BuildingKind{House, FoodSource, WaterSource, FireStation, ArchitectPost}`.
`WalkerKind{Food, Water, Fireman, Architect}`.
The three are welded together by arithmetic: `walkerSentBy(kind)` is `WalkerKind(buildingKindIndex(kind) - 1)`, `carriedResource(kind)` is `Resource(walkerKindIndex(kind))`, `buildingKindOf(tool)` is `BuildingKind(buildToolIndex(tool) - 1)`, and three `static_assert`s pin the counts to each other.

`Building` holds `stock[kResourceCount]`, `risk`, three countdowns (`ticksUntilSpawn`, `ticksUntilDrain`, `ticksUntilRisk`) and one `ecs::Entity walker`.
`BuildingSystem::update()` builds a `std::map<Cell, Entity>` of every cell every block stands on, runs `deliver()` (every walker gives to every adjacent building, accumulating into a `std::map<Entity, Building>` so two walkers add rather than race), then `age()` (drain and risk), then writes or demolishes.
`SpawnSystem` lets a building send **one** walker at a time, held by handle, with `world.alive()` as the authority.
`WalkerSystem` roams by `nextFacing()`'s single preference order (right, straight, left, back) and heads home via `stepTowards()`, a full A* re-run every step of which only the first move is used.
Constants: `kTicksPerSecond` 25, `kStockCapacity` 100, `kMaxRisk` 100, `kRiskRelief` 25, `kRoamingSteps` 32, `kWalkerLoad` 100, `kTicksPerStep` 2, `kWalkerLimit` 64.

### What is saved, and at what version

`kSaveMagic` is `"antwika-game-save"`; `kSaveFormatVersion` is **2**.
`standardSaveMigrations()` holds one migration, `AddBuildings` (1 -> 2).
`SaveGame{state, extent, camera, paths, walkers, buildings, seed}`, encoded in `src/SaveGame.cpp` against a schema built in the same file, read `parse -> version -> migrate -> validate -> decode` through `replay::readVersionedDocument<SaveFormatError>`.
The building/walker link is a **pair of array indices**, and `requireConsistentLinks()` refuses an out-of-range or disagreeing pair rather than repairing it.
`SessionStore` is the one door: `take()` for `--save` and the Save button, `restore()` for `--load` and the Load button.

### The world map

`generateWorldMap(WorldMapConfig)` solves terrain with `antwika::wfc` from a lattice of seed-drawn anchors, then `placeCities()` picks four sites by three integer comparisons.
The whole world is a pure function of the seed, which is why `SaveGame::seed` exists.

### Rendering

`RenderSystem` implements both `ecs::ISystem` and `app::IFramePass`, snapshotting in `update()` and redrawing in `draw()`.
`SceneSnapshot` is a plain comparable value; `snapshotOf()` fills everything except `ghost` and `hover`, and sorts buildings on `x + y` then `x`.
`WalkerView`/`BuildingView` are state; `WalkerSprite`/`BuildingSprite` are the picture.
`ghostFor()` and `hoverFor()` read `input::PointerHintChannel` and may decide only what is drawn.
`TileAtlas.hpp` addresses a 1024x256 sheet arithmetically: slot 0 ground, 1-16 roads by link mask, 17-20 walkers by `Direction`, 21-25 buildings by `BuildingKind`, 26-31 spare.

### Tests

About a hundred files under `src/apps/game/tests/`, all windowless: the test target links `antwika::gfx::tests::mocks` and never a real backend.
The determinism spine is `ReplayDeterminismTest` (record a run, replay it, compare `GameSummary`), `FrameRateDeterminismTest`, `PauseDeterminismTest` and `HoverTest`.

### i18n

`apps/game` does not link `antwika::i18n` and does not use it.
`toolLabel()`, `pauseLabel()`, `"zoom "`, `"tick "` and every menu caption are raw English literals, even though `MessageId::ToolbarZoomIn`, `ToolbarZoomOut`, `ToolbarResetView` and `ToolbarZoomLevel` already exist in the catalogue with Swedish translations.
That is a standing debt this increment pays.

## 2. The gap

### Pillars already present in some form

- **Walkers on roads, sent by buildings.** The spine exists: spawn, roam, deliver, head home, expire.
- **A building that must be reached to work.** `spawnCellFor()` and `stepTowards()` already treat a block as reachable by any of its cells.
- **A crude service.** The fireman and the engineer already relieve `risk` by walking past, which is coverage in disguise and is the right shape to generalise.
- **Consumption and consequence.** A house drains stock and is lost when a resource hits zero or risk hits maximum.
- **A world above the city.** The world map, four cities, per-city indices.
- **Versioned, migrating saves.** Complete, and already carries per-building countdowns for the right reason.

### Pillars absent entirely

Housing tiers.
Population and immigration.
Labour allocation and employment.
Desirability.
Service coverage as a thing distinct from goods delivery.
Storage, markets and workshops -- no raw -> workshop -> market -> consumption chain exists.
Ratings.
External demands.

### Existing decisions that would fight a walker simulation

These are the ones a workstream must change rather than work around.

1. **`Building::walker` is one handle and `SpawnSystem`'s whole rule is "one out at a time".**
   A market sends a buyer *and* a seller; a workshop sends a cart pusher while a labour seeker is out.
   This is the single hardest blocker and it reaches the save file, because the link is persisted.
2. **`walkerSentBy()`, `carriedResource()` and `buildingKindOf()` are arithmetic identities**, guarded by `static_assert(kWalkerKindCount == kBuildingKindCount - 1)` and `static_assert(kBuildToolCount == kBuildingKindCount + 1)`.
   They are elegant exactly while every source sends one walker and every tool is a building.
   Round 1 breaks both premises.
3. **`BuildingSystem::deliver()` gives to every adjacent building unconditionally.**
   A cart pusher bound for a specific store must not shed its load into a house it walks past.
4. **`Building::stock` is baked into the save schema** with `minItems == maxItems == kResourceCount`, so growing `Resource` is a breaking change.
5. **`ecs::View` order is "whichever storage is smallest".**
   It is reproducible run-to-run -- which is all replay determinism needs today -- but it is not a total order over anything a reader can name, and it *changes* as component counts cross each other.
   Any decision that splits a limited amount among several actors must not depend on it.
   Today `deliverTo()` clamps at capacity, so with two walkers the split already depends on view order; it is deterministic only because the histories match.
6. **The atlas has six spare slots and a fixed `kAtlasRows`.**
   Round 1 wants ten building kinds.
7. **`WalkerSystem::headHome()` runs a full A* per walker per step and uses one move of it.**
   At `kWalkerLimit` 64 that is fine; at Caesar's walker densities it is the cost centre.
   Round 1 does not fix it and must not make it structurally harder to fix.
8. **The palette row is a loop over `kBuildToolCount` and `widgets::kMenu` is derived from it.**
   Doubling the tools changes the bar's layout, and a recorded click resolves against the layout.
   `src/apps/game/replays/demo.json` and `ReplayIntegrationTest` are what will notice.
9. **`GameState::score` is the only rating-shaped number and it is fed by an event.**
   A genuine city rating is a pure function of the city and must never acquire an event.
10. **No i18n.**
    Every new caption would otherwise be another English literal.

## 3. The rules this increment is written under

Stated here because every workstream is bound by them, and breaking one looks fine live and surfaces as a divergent replay a long way from its cause.

- **`simulation::EngineLoop` is the one code path.**
  Live and replay differ only in what implements `ITickEventSource`.
  No workstream may add a second loop, a second dispatcher, or a system that reads a clock.
- **Only externally-supplied input is persisted.**
  A click is recorded; the tile it lays, the walker it spawns, the goods it moves, the labour it allocates and the rating it changes are all regenerated.
  **This increment adds zero new `game.*` event kinds**, and each workstream below names the tempting candidate it rejected and why.
- **Anything the meaning of a click depends on is simulation state.**
  The camera, the selected tool, the mode, the pause and the road drag all already are.
  Any new thing a click's meaning depends on -- a selected overlay, a chosen priority -- joins them, written inside the tick path and never persisted as an event.
- **Rendering is a write-only projection.**
  No pixel, measurement or float from the render side enters the tick loop.
  Hover may decide what is drawn and nothing else.
  New numbers that exist only to be looked at go on `BuildingSprite`/`WalkerSprite`; new numbers that are state go on `BuildingView`/`WalkerView` and into `GameSummary`.
- **A UI is described and resolved inside the tick path**, downstream of the recorder.
  No `ui.*` event name may ever exist.
  A new panel that is driven by hover is painted through `IRenderer` like `readoutPanel()` is, never through `antwika::ui`.
- **Every persisted schema states its version** and migrates through single-step `replay::MigrationChain` migrations.
  See section 4 for how this increment keeps that to **one** bump.
- **Determinism is total ordering.**
  Every new ordered decision in section 5 names the total order it breaks down to.
  Randomness only from an injected `antwika::rng` seeded from something already persisted -- and **round 1 introduces no new randomness at all**, so every new decision is a total order over integers.
  When randomness does arrive, the rule this increment fixes is: derive a `SplitMix64Rng` per decision from `(SaveGame::seed, tick, Cell)` rather than advancing one shared stream, because a shared stream makes the answer depend on which system drew first.
- **One exception type per failure category**, declared by the module that owns the failure.
  Round 1 adds none: everything new is either a total function or refused by the existing `SaveFormatError`.
- **100% line, function and branch coverage on the GNU leg.**
  Every workstream is testable without a window; see the per-workstream test lists and section 6.
- **Player-facing text goes through `antwika::i18n` as symbolic `MessageId`s.**
  W1 links the library and threads a `const Translator &`; W2-W5 append their own ids to the central enum and to *both* catalogues.
- **Design rationale lives on `wiki/apps/game.md`**, not in `CLAUDE.md` and not in this plan once it is deleted.

## 4. The save format, decided once

This is the decision that keeps four workstreams from fighting over one file, so it is stated before the workstreams.

**The increment bumps the save format exactly once, from 2 to 3, and W1 owns that bump.**

`docs/schema-versioning.md` says an additive change needs no bump: "Adding an optional member is the usual case -- `"canvas"` was one, and did not bump the version."
Every piece of state W2, W3, W4 and W5 add is genuinely additive, because **absent means the value the game had before that feature existed**: no coverage, level zero, no population, no production.
That is the same idiom `SavedWalker::home` already uses through `linkFromJson()`.
So W2-W5 add optional members with documented defaults and write **no migration at all**.

W1's changes are genuinely breaking and take the one bump:

- `SavedBuilding::walker` becomes an array of indices, because a building may now have more than one walker out.
- `Building::stock` changes width, because `Resource` changes.
- The `kind` strings change, because `BuildingKind` and `WalkerKind` are renamed and re-populated.

The v2 -> v3 migration therefore: wraps `walker` in an array; rewrites each `stock` from `[food, water]` to `[food, 0, 0]`, which is the honest reading now that water is a service rather than a stored good; renames `"food_source"` -> `"farm"`, `"water_source"` -> `"well"`, `"architect_post"` -> `"engineer_post"`; and renames walker kinds `"food"` -> `"market_seller"`, `"water"` -> `"water_carrier"`, `"architect"` -> `"engineer"`.
It is tested against a **hand-written literal version-2 document**, as step 4 of `docs/schema-versioning.md` requires, not against one this build produced.

If a workstream later discovers it must *tighten* something rather than add, it takes version 4 and writes a 3 -> 4 migration; the merge order in section 6 fixes who would get 4.

## 5. The increment: five workstreams

### W1 -- Vocabulary and seams

**Goal.**
Land, in one mechanical change with no new gameplay, everything the other four workstreams would otherwise each have to edit: the full round-1 enumerations and their tables, the decoupling of the three arithmetic identities that weld them together, a building that can have more than one walker out, the atlas slots and art contract for ten building kinds, the `antwika::i18n` wiring, and the one save bump to version 3.
Nothing here changes what the city *does* beyond the renames; a run before and after should place the same things and reach the same summary modulo the new names.
This workstream is the increment's only serialisation point and it must be finished fast.

**Files and types.**

- `include/antwika/game/Resource.hpp` -- `Resource{Food, Clay, Pottery}`; water leaves, because a well confers coverage rather than stock.
  `kResources` and `resourceName()` follow.
- `include/antwika/game/Service.hpp` -- **new**: `Service{Water, Health, Safety, Structure}`, `kServiceCount`, `serviceIndex()`, `serviceName()`, `kServices`, with the same index-order `static_assert` `Resource.hpp` uses.
- `include/antwika/game/BuildingKind.hpp` -- `BuildingKind{House, Farm, ClayPit, Workshop, Storage, Market, Well, Doctor, FireStation, EngineerPost}`.
  `consumes()` and `sendsWalkers()` become tables rather than `kind == House` and its negation, because `Storage` sends nobody and `Market` sends two kinds.
- `include/antwika/game/Walker.hpp` -- `WalkerKind{WaterCarrier, Doctor, Fireman, Engineer, CartPusher, MarketBuyer, MarketSeller}`.
  `walkerSentBy()` and `carriedResource()` become explicit tables; the `kWalkerKindCount == kBuildingKindCount - 1` `static_assert` goes.
- `include/antwika/game/BuildTool.hpp` -- one tool per building kind plus `Road`; `buildingKindOf()` becomes an explicit table; the `kBuildToolCount == kBuildingKindCount + 1` `static_assert` goes.
- `include/antwika/game/Footprint.hpp` -- the ten-entry `kFootprints` table.
  House/Well/Doctor/FireStation/EngineerPost 1x1, Farm/ClayPit/Workshop/Market 2x2, Storage 3x3.
  The "square only" `static_assert` stays and is the reason those are the sizes.
- `include/antwika/game/Building.hpp` -- `walker` becomes `std::array<ecs::Entity, kMaxWalkersOut> walkers` with `kMaxWalkersOut = 2`.
  A fixed array rather than a vector, because `ecs::Component` requires trivially copyable and standard layout.
  `SpawnSystem` gains a helper `freeWalkerSlot(world, building)` and its "one out at a time" rule becomes "one out per slot", with `world.alive()` still the authority and `EntityManager`'s never-reused indices still the reason a stale handle can only be dead.
- `include/antwika/game/TileAtlas.hpp` -- `kAtlasRows` raised to 5 (a 1024x320 sheet), building slots 21-30, spare 31-39.
  `requireAtlasSize()` in `src/AtlasImage.cpp` and its message follow.
- `assets/atlas.png` -- repainted.
  Flat placeholder diamonds for the five new kinds, respecting the diamond contract, with the art debt recorded in the wiki.
- `wiki/apps/game-texture-atlas.md` -- the slot table, the sheet size and the footprint table are the contract with whoever draws the art, and all three change.
- i18n: `src/libs/i18n/include/antwika/i18n/MessageId.hpp` gains `GameTool*` ids and `kAllMessageIds` gains them; `src/libs/i18n/src/MessageId.cpp` and both catalogues in `src/libs/i18n/src/Catalogue.cpp` follow.
  `src/apps/game/CMakeLists.txt` and `tests/CMakeLists.txt` link `antwika::i18n`.
  `main.cpp` constructs one `Translator` at `kDefaultLocale` -- **fixed in source, never from a flag or an environment variable**, because this application hit-tests a layout laid out from translated text.
  `Toolbar::describe()`, `UiSink`, `MainMenuScene`, `MenuModalScene`, `SaveLoadScene` and `ReadoutPanel` take a `const Translator &`.
  `toolLabel()` and `pauseLabel()` return a `MessageId` rather than a `std::string_view`.
- Save: `src/SaveGame.cpp` decodes `walkers` as an array and pads `stock`; `requireConsistentLinks()` generalises to the array; `src/SaveMigrationV2ToV3.cpp` is a **new file** holding the migration, and `src/SaveMigration.cpp` gains one `make_shared` line.
  `kSaveFormatVersion` becomes 3.
- Save sections: split `src/SaveGame.cpp` into a spine plus per-section encode/decode functions declared in a new private `src/SaveSections.hpp`.
  This is what lets W2-W5 each add a self-contained pair of functions in a file of their own and touch the spine only twice.
- `src/apps/game/replays/demo.json` re-recorded if the toolbar layout moves.

**New event kinds.**
None.
The tempting candidate is `game.select_tool`, since the palette doubles in size.
It is rejected for the reason `Events.hpp` already gives at length: the selection is regenerated from the recorded click by `UiSink` inside the tick path, and persisting it would select twice per press.

**Save-schema migration.**
Version 2 -> 3, as specified in section 4.
Tested with a hand-written v2 document, including one with a null `walker`, one with a scalar `walker`, and one naming `"water_source"`.

**Tests.**
`BuildingKindTest`, `BuildPaletteTest`, `WalkerTest`, `FootprintTest` and `TileAtlasTest` extended to the new tables -- exhaustively, since these *are* the feature.
`SaveMigrationTest` gains the v2 -> v3 cases.
`SaveGameTest` round-trips a two-walker building.
`SpawnSystemTest` gains "a market with two free slots sends two walkers" and "a full building sends none".
A new `ToolLabelTest` asserting every `BuildTool` has a distinct `MessageId` present in both catalogues.
`ReplayIntegrationTest` re-run against the re-recorded fixture.

**Dependencies.**
None.
**Every other workstream depends on this one**, and none of W2-W5 may edit `Resource`, `Service`, `BuildingKind`, `WalkerKind`, `BuildTool`, `Footprint`, `TileAtlas` or the atlas art.

---

### W2 -- Service coverage, desirability and risk

**Goal.**
Turn "a walker hands over a number" into the Impressions model, where most walkers confer *coverage*: a service walker passing a building tops that building's coverage for its service up to full, and coverage decays with time, so a district is served only while walkers keep reaching it.
Convert the existing fire and engineer relief into that model rather than leaving it beside it -- risk grows only where `Service::Safety` or `Service::Structure` coverage has lapsed, so the fire station and the engineer's post stop being special cases.
Add desirability as an integer field over the grid, summed from what stands nearby, since it is what every later pillar reads.

**Files and types.**

- `include/antwika/game/Coverage.hpp` -- **new** component `Coverage{std::array<std::int32_t, kServiceCount> ticksLeft{}}`, plus `kCoverageFull` and `coverageOf(world, entity, Service)` returning zero for a building with no component.
- `include/antwika/game/ServiceWalk.hpp` -- **new**: `serviceConferredBy(WalkerKind) -> std::optional<Service>`, a table.
- `include/antwika/game/CoverageSystem.hpp` / `src/CoverageSystem.cpp` -- **new** `ecs::ISystem`.
  Tops up from passing walkers, decays every `kCoverageDecayPeriodTicks`, and ages `risk` only where the relevant coverage is zero.
- `include/antwika/game/Desirability.hpp` / `src/Desirability.cpp` -- **new** `DesirabilityField`, a `std::map<Cell, std::int32_t>` rebuilt from the buildings, with `kDesirabilityOf` a per-`BuildingKind` table of (contribution, radius) and a linear integer falloff over Chebyshev distance.
  Owned by `bootstrap()` beside `PathIndex` and `BuildingIndex`, and passed to `RenderSystem` the same way.
- `src/BuildingSystem.cpp` -- `age()` loses its unconditional risk arm; the risk rule moves to `CoverageSystem`.
  `deliver()` loses the fireman/engineer relief arm.
- `src/Game.cpp` -- a `"serve"` phase after `"walk"`, holding `CoverageSystem` and `DesirabilitySystem`, each `SessionGatedSystem` then `PauseGatedSystem`.
- `SceneSnapshot.hpp` -- `BuildingView` gains `coverage`, because it is state the summary should compare, and `printSummary` reports it; `BuildingSprite` gains it too so `ResourceBar.hpp` can gauge it.
- `src/ReadoutPanel.cpp` -- the hover panel lists coverage beside stock.
- `src/SaveCoverage.cpp` -- **new**, the section encode/decode declared in `src/SaveSections.hpp`.

**New event kinds.**
None.
The candidate is `game.coverage_lapsed`, so a UI could react to it.
Rejected: coverage is a function of walkers, which are a function of buildings, which are a function of clicks.
It is regenerable at every step, and a notification is a picture rather than an input.

**Save-schema migration.**
None.
`SavedBuilding` gains an optional `"coverage"` array of `kServiceCount` counts; absent means zero, which is exactly what a version-3 file written before coverage existed means.
Additive per `docs/schema-versioning.md`, so no bump.

**Tests.**
`CoverageSystemTest` -- a walker adjacent to a building tops that service and no other; coverage decays to zero and no further; two walkers of one service in one tick leave the same value as one, because the aggregation is `std::max` and therefore idempotent.
`DesirabilityTest` -- the field is a pure function of the building set, and the same set in two insertion orders gives the identical field.
`BuildingSystemTest` -- risk grows with no safety coverage and does not with it.
`ReplayDeterminismTest` extended to a run long enough for coverage to lapse.

**Dependencies.**
W1 only.
W3 reads `coverageOf()`, so W2's first commit must be `Coverage.hpp` alone.

---

### W3 -- Housing evolution

**Goal.**
Give a house a level, and make that level the thing the whole city is arranged to raise.
A house evolves when it has held the next level's requirements -- coverage of the named services, stock of the named goods, and desirability at or above a threshold -- for a sustained countdown, and devolves on the same terms in reverse.
Its level decides how much population it holds and what it demands next, which is what turns a district into a feedback loop rather than a set of independent buildings.
No merging of 1x1 houses into blocks in this increment.

**Files and types.**

- `include/antwika/game/HousingLevel.hpp` -- **new**: `HousingLevel{Tent, Shack, Hovel, Cottage}` (four is enough to show the loop), `kHousingLevelCount`, `housingLevelName()`, and `kHousingRequirements`, a table of `{desirability, services, goods, populationCapacity}` per level.
  **This header is the seam W4 and W5 read, so it is published on day one.**
- `include/antwika/game/Household.hpp` -- **new** component `Household{HousingLevel level, std::int32_t ticksUntilEvolve, std::int32_t ticksUntilDevolve, std::int32_t population}`.
  `population` lives here rather than in W4's own component because a level's capacity and its occupancy are one fact; W4 owns the *rules* that change it and W3 owns the storage.
- `include/antwika/game/HousingSystem.hpp` / `src/HousingSystem.cpp` -- **new** `ecs::ISystem`, in a `"settle"` phase after `"produce"`.
- `include/antwika/game/HousingQuery.hpp` -- `levelOf(world, entity)` and `capacityOf(HousingLevel)`, the read-only face W4 uses.
- `src/SceneSnapshot.cpp` -- `BuildingView` gains `level`; `BuildingSprite` gains it too, so the art can differ by tier once there is art for it.
- `src/GridScene.cpp` -- a house draws its level's slot once the atlas has one; until then it draws the house slot and the level shows only in the hover panel.
- `src/ReadoutPanel.cpp` -- the panel names the level and what the next one wants.
- `src/SaveHousing.cpp` -- **new** section.
- `src/Game.cpp` -- the `"settle"` phase and one `addSystem`.

**New event kinds.**
None.
The candidate is `game.house_evolved`, which reads like a notification worth recording.
Rejected: it is a pure function of coverage, stock and desirability, all of which a replay regenerates, so recording it would evolve the house twice.

**Save-schema migration.**
None.
`SavedBuilding` gains optional `"level"`, `"population"` and the two countdowns; absent means `Tent`, zero population and a fresh countdown, which is what a house in a version-3 file was.
Note that the countdowns are persisted rather than reset for the reason the existing three are -- resetting them is the lockstep they exist to avoid.

**Tests.**
`HousingSystemTest` -- a house meeting a level's requirements for exactly the countdown evolves and not a tick sooner; one falling short devolves; a house at the top level does not evolve past it; a house at the bottom does not devolve below it.
Requirements are read from the table, so the test is a loop over `kHousingRequirements` rather than four hand-written cases.
`SaveGameTest` round-trips a mid-evolution house.
`ReplayDeterminismTest` extended so a recorded run actually evolves something, which is what makes `BuildingView::level` earn its place in the comparison.

**Dependencies.**
W1, plus the headers `Coverage.hpp` (W2) and `Store.hpp` (W5).
Until those merge, `HousingSystem` reads through their query functions and treats an absent component as zero, which is both the correct semantics and what lets W3 build and pass before either has landed.
Merges after W2 and W5.

---

### W4 -- Population, immigration, labour and ratings

**Goal.**
Make the city's people the resource everything else competes for.
A house with a road beside it, desirability at or above its level's threshold and free capacity gains population on its own countdown; a house below the threshold loses it.
The sum of population is a workforce; every workplace declares how many workers it wants; the workforce is allocated in a total order and a workplace short of workers produces and spawns proportionally slower or not at all.
Add the city's ratings as a pure function of that state, drawn on the toolbar -- the first thing that judges the city rather than simulating it.

**Files and types.**

- `include/antwika/game/Workforce.hpp` -- **new** component `Workforce{std::int32_t wanted, std::int32_t employed}` on every non-house building, and `kWorkersWanted`, a per-`BuildingKind` table.
- `include/antwika/game/LabourSystem.hpp` / `src/LabourSystem.cpp` -- **new** `ecs::ISystem`.
  Sums population, then allocates in **ascending `Cell` of the workplace's origin**; see the ordering note in section 6.
- `include/antwika/game/PopulationSystem.hpp` / `src/PopulationSystem.cpp` -- **new** `ecs::ISystem`, in the `"settle"` phase after `HousingSystem`.
- `include/antwika/game/LabourQuery.hpp` -- `workersAt(world, entity)` and `staffingOf(world, entity)` returning a numerator/denominator pair of integers, never a float, which is what W5's production and `SpawnSystem`'s cadence scale by.
- `include/antwika/game/CityRatings.hpp` / `src/CityRatings.cpp` -- **new** pure function `ratingsOf(const World &, const DesirabilityField &) -> CityRatings{population, employment, averageHousingLevel, serviceReach}`, every member an integer, nothing persisted and nothing evented.
- `src/Toolbar.cpp` -- two labels reading the ratings, through i18n ids, described in the tick path off state a replay regenerates, exactly as the tick label already is.
  **Appended after `widgets::kMenu` on the first row**, so every existing widget keeps its place and a recording made before this still hits the same buttons.
- `src/SpawnSystem.cpp` -- the spawn countdown scales with `staffingOf()`, integer arithmetic only.
- `src/SaveLabour.cpp` -- **new** section.
- `GameSummary` -- gains `CityRatings`, which makes a divergence in population or employment fail `ReplayDeterminismTest` directly.

**New event kinds.**
None, and this is the workstream where one is most tempting.
The candidate is `game.immigrant_arrived` or a `game.set_wage` from a UI.
Both are rejected: the first is a function of desirability and capacity, and the second is a click on a widget resolved by a sink inside the tick path, which is exactly what "no `ui.*` event name may ever exist" means.

**Save-schema migration.**
None.
`SavedBuilding` gains an optional `"employed"`; population rides on W3's `"population"`.
`employed` absent means zero, which is what a version-3 file's workplaces were.
The workforce total and the ratings are **not** persisted, because both are sums over what is.

**Tests.**
`LabourSystemTest` -- a workforce short of demand is allocated in ascending `Cell` order and the same city built in a different creation order allocates identically.
That last case is the new `AllocationOrderTest` described in section 6 and is the most valuable test in the increment.
`PopulationSystemTest` -- a house with no road beside it gains nobody; one over capacity gains nobody; one below its desirability threshold loses population and does so down to zero and no further.
`CityRatingsTest` -- a pure-function table test over hand-built worlds, no renderer.
`ToolbarTest` -- the ratings labels are described, asserted through `ui::Frame::rects` and `tests/WidgetPixel.hpp` rather than by sweeping the canvas.

**Dependencies.**
W1, plus `HousingLevel.hpp`/`HousingQuery.hpp` (W3) and `Desirability.hpp` (W2).
Merges last.

---

### W5 -- Production, storage, carts and markets

**Goal.**
Build the chain the genre is about: a farm and a clay pit produce raw goods into their own stock; a cart pusher carries a full load to a named storage rather than roaming; a workshop pulls clay from storage, turns it into pottery and carts it back; a market sends a buyer to storage and a seller to roam the houses handing goods out.
That is raw material -> workshop -> market -> consumption, end to end, and it is the first time a walker in this application has a *destination* rather than a preference order.

**Files and types.**

- `include/antwika/game/Production.hpp` -- **new** component `Production{std::int32_t ticksUntilOutput}`, and `kProduces` / `kConsumesToProduce`, per-`BuildingKind` tables of `std::optional<Resource>`.
- `include/antwika/game/Store.hpp` -- **new**: `acceptsAt(BuildingKind, Resource)`, `stockOf(world, entity, Resource)`, and `kStoreCapacity`.
  Storage accepts every resource; a market holds what it bought; a house holds what a seller gave it.
- `include/antwika/game/Errand.hpp` -- **new** component `Errand{ecs::Entity destination, Resource carrying, ErrandLeg leg}` where `ErrandLeg{Outbound, Returning}`.
  A walker with an `Errand` is routed; one without it roams, which keeps `WalkerSystem` unchanged for every existing walker.
- `include/antwika/game/ErrandRouting.hpp` / `src/ErrandRouting.cpp` -- **new**: `nearestAccepting(...)` and `nearestHolding(...)`, each returning the destination entity, with the ordering rules in section 6.
- `include/antwika/game/ProductionSystem.hpp`, `HaulingSystem.hpp`, `MarketSystem.hpp` and their `.cpp`s -- **new**, in a `"produce"` phase after `"serve"`.
- `src/WalkerSystem.cpp` -- one new arm: a walker with an `Errand` steps toward its destination through the existing `stepTowards()` rather than through `nextFacing()`, and on arrival hands over and flips its leg.
  A destination that has died or been walled off falls into the existing "destroyed, and that is not an error" arm, which is the whole reason that arm was written to absorb every awkward case.
- `src/BuildingSystem.cpp` -- `deliver()` gains one guard: a walker carrying an `Errand` gives to its destination and to nothing it walks past.
- `src/SaveProduction.cpp` -- **new** section, covering both the building's production countdown and the walker's errand.
- `SceneSnapshot` -- `WalkerSprite` already carries `kind` and `carried`; `BuildingSprite` gains the store's stock per resource so `ResourceBar` can gauge a granary.

**New event kinds.**
None.
The candidate is `game.goods_delivered`, so a market could react without polling.
Rejected outright: it is derived from a walker's position, which is derived from a route, which is derived from a click.
`Events.hpp` already states the rule and this is the same rule.

**Save-schema migration.**
None.
`SavedBuilding` gains an optional `"ticksUntilOutput"`; `SavedWalker` gains an optional `"errand"` object holding a destination index, a resource name and a leg.
Absent means no production and no errand, which is what a version-3 file held.
The destination index is validated exactly as `home` is, by an extension of `requireConsistentLinks()` -- an index past the end of the buildings array is refused rather than repaired, because a repaired save is a session somebody never had.

**Tests.**
`ProductionSystemTest` -- a farm produces on its countdown and holds at capacity; a workshop with no clay produces nothing.
`ErrandRoutingTest` -- nearest-accepting picks by path length and breaks ties by ascending `Cell`; a store that is full is not chosen; no reachable store yields no errand and no walker.
`HaulingSystemTest` -- a cart pusher walks past a house without shedding its load, which is the guard's whole purpose.
`MarketSystemTest` -- a seller supplies adjacent houses in ascending `Cell` order and the split of a partial load is identical under two creation orders.
`WalkerSystemTest` -- an errand walker whose destination is demolished mid-route is destroyed and nothing throws.
`SaveGameTest` -- round-trips a walker mid-errand and refuses a bad destination index.

**Dependencies.**
W1, plus `LabourQuery.hpp` (W4) for staffing.
Production treats an absent `Workforce` as fully staffed, so W5 builds and passes before W4 lands, and W4's merge is what makes staffing bite.
Merges after W2, before W3.

## 6. The seams between workstreams

This is the section that makes parallel implementation possible.
Read it before starting, not after a conflict.

### The one rule that makes everything else work

**Every new component is optional, and its absence is the value the game had before that workstream existed.**
No coverage means uncovered.
No `Household` means level `Tent` and nobody living there.
No `Workforce` means fully staffed.
No `Production` means producing nothing.
No `Errand` means roaming.

This buys three things at once.
A workstream compiles and passes its tests before its dependencies merge.
The game is playable at every merge point rather than only at the end.
And the save format needs no migration for any of it, per section 4.

### Files more than one workstream touches, and how they are divided

**`src/Game.cpp`** -- W2, W3, W4, W5.
Each adds **one contiguous block**: its system constructions, its two gate wrappers each, one `createPhase`, its `addSystem` calls.
The phase order is fixed below and no workstream may insert above another's block, so the blocks append and git merges them.

**`src/SaveGame.cpp`** -- W1 splits it; W2-W5 add **two lines each**.
W1 extracts the spine and declares per-section functions in `src/SaveSections.hpp`.
Each later workstream writes its section in a **file of its own** (`SaveCoverage.cpp`, `SaveHousing.cpp`, `SaveLabour.cpp`, `SaveProduction.cpp`), adds one declaration to `SaveSections.hpp` and one call to each of the spine's encode and decode.

**`src/SaveMigration.cpp`** -- W1 only.
Only W1 bumps the version in this increment.
`MigrationChain` looks a step up by `fromVersion()`, so list order is not semantic and a future append is conflict-free.

**`include/antwika/game/SceneSnapshot.hpp`** -- W2, W3, W5.
Each appends its own member to `BuildingSprite` and/or `BuildingView`, at the end, and nothing reorders.
The state/picture split is per section 3: a number a replay must agree on goes on the view, a number that exists only to be looked at goes on the sprite.

**`src/SceneSnapshot.cpp`** -- the same three.
Each fills in its own member in the existing loops.
The `x + y` then `x` sort is untouched.

**`src/GridScene.cpp`, `src/ResourceBar.cpp`, `src/ReadoutPanel.cpp`** -- W2, W3, W5.
Each adds one drawing pass or one panel line, after the existing ones.
Bars are still worked out from `footprintBounds()` and `walkerBounds()` so the gauges cannot become a second layout.

**`src/Toolbar.cpp` / `Toolbar.hpp`** -- W1 for the labels and the palette, W4 for the ratings labels.
W1 owns the palette and its layout; W4 appends its labels **after `widgets::kMenu` on the first row**, so every existing widget keeps its place.
Nobody else touches the bar.

**`MessageId.hpp`, `MessageId.cpp`, `Catalogue.cpp`** -- W1, W2, W3, W4.
Each **appends** its ids to the enum, to `kAllMessageIds`, to the name switch and to **both** catalogues.
Appending rather than inserting is a merge convenience only -- a `MessageId` is never persisted, so its numbering is free.
`CatalogueTest.CatalogueFor_CoversExactlyTheSameIdSetInEveryLocale` is what fails if a workstream forgets the Swedish.

**`CMakeLists.txt` and `tests/CMakeLists.txt`** -- all five.
Alphabetically ordered source lists; each appends its own files.
This is the most frequent and most trivial conflict in the increment.

**`wiki/apps/game.md`** -- all five.
Each appends **one `##` section** describing its own rationale, before the existing `## Future work` and `## See also`.
Nobody edits another's.

**`assets/atlas.png`, `TileAtlas.hpp`, `wiki/apps/game-texture-atlas.md`** -- W1 only, and this is a hard rule.
A second workstream repainting the sheet is a binary conflict nobody can resolve.

**`Resource.hpp`, `Service.hpp`, `BuildingKind.hpp`, `Walker.hpp`, `BuildTool.hpp`, `Footprint.hpp`** -- W1 only, and this is the reason W1 exists.
Renumbering a shared enum under another workstream is how a save file silently means something else.

### The phase order, fixed here

`Game.cpp` runs phases in creation order, and a phase commits at its end, so a later phase sees what an earlier one wrote *this* tick.

1. `"walk"` -- `WalkerSystem`, `BuildingSystem`, `SpawnSystem`, **unchanged**.
   Their sharing of one phase is documented on the wiki and `SpawnSystem` depends on it.
2. `"serve"` -- W2's `CoverageSystem`, then `DesirabilitySystem`.
3. `"produce"` -- W5's `ProductionSystem`, `HaulingSystem`, `MarketSystem`.
4. `"settle"` -- W3's `HousingSystem`, then W4's `PopulationSystem` and `LabourSystem`.
5. `"observe"` -- the observers, **unchanged**.

Every new system is wrapped `SessionGatedSystem` then `PauseGatedSystem`, in that order, because a city runs while its player reads the world map and stops only where a player asked -- and because either gate alone answers only its own question.

### Publish-the-header-first, and the merge order

Development is parallel; **merging is ordered**, because three workstreams read each other's queries.

Merge order: **W1 -> W2 -> W5 -> W3 -> W4.**

Each of W2, W3, W4 and W5 pushes its **public header alone** as its first commit -- `Coverage.hpp`, `HousingLevel.hpp` + `HousingQuery.hpp`, `LabourQuery.hpp`, `Store.hpp` -- before writing a line of system code.
A header with a query that answers the absent-component default is enough for a consumer to build against, and it costs the publisher nothing.

### Total ordering: how each new decision is made total

This is the hardest constraint for a walker simulation, so each new decision names its order explicitly.

**The finding that drives all of it.**
`ecs::View` documents its order as "whichever storage has the fewest entities", which is stable for a given history but flips as component counts cross each other, and is not a property of the grid that anybody can name.
It is fine for a loop whose body is independent per entity.
It is **not** fine for anything that splits a limited amount, and `BuildingSystem::deliverTo()` already clamps a limited amount inside such a loop.

**The rule.**
Any system whose outcome depends on *which actor goes first* builds an explicit `std::map` keyed by the actor's `Cell` -- and by its `ecs::Entity` where two actors can share a cell, since walkers do not collide -- and iterates that.
Any system whose per-entity effect is independent, idempotent or commutative may use `view()` directly, and should say in a comment which of the three it is.

Decision by decision:

- **Coverage top-up (W2).** `std::max` against `kCoverageFull`.
  Idempotent, so two walkers in one tick give what one gives and order cannot matter.
- **Coverage decay and risk (W2).** Per building, from its own component.
  No cross-entity ordering exists.
- **Desirability (W2).** A sum of integer contributions over buildings.
  Commutative and associative, so the field is a pure function of the building *set*.
  The test asserts exactly that, by building the same set twice in two orders.
- **Housing evolve/devolve (W3).** Per house, from its own state.
  No cross-entity ordering.
  Explicitly: **no house merging in this increment**, because merging is the one housing rule that would need a total order over neighbours.
- **Immigration (W4).** Per house, bounded by that house's own capacity.
  There is no shared migrant pool in round 1 -- deliberately, and this is why.
- **Labour allocation (W4).** The workforce is city-wide and limited, so this is the increment's one genuinely contended allocation.
  Workplaces are walked in **ascending `Cell`** of their origin, which `Cell::operator<=>` already provides as a `std::strong_ordering` and which `std::map` already iterates.
  **No tie-break is needed at all**, because `BuildingIndex` guarantees two buildings cannot share an origin cell -- the strongest available form of a total order.
- **Cart destination (W5).** `nearestAccepting()` orders by path length from the producer's spawn cell, then by ascending `Cell` of the store's origin.
  The path length comes from `antwika::pathfinding`, whose open set already orders down to ascending `NodeId`, and the `GridGraph` is built over the **configured `GridExtent`**, never a bounding box of the roads -- a bounding box would renumber every node as a road was laid, and with it the tie-break.
  That is the same rule `Homing.cpp` and `RoadPlan.cpp` already follow, and W5 must not invent a second one.
- **Market seller handing goods out (W5).** A partial load split among adjacent houses.
  Recipients are collected into a `std::map<Cell, Entity>` and served in ascending `Cell`; sellers are collected into a `std::map<std::pair<Cell, Entity>, ...>` and applied in that order, accumulating into the existing `std::map<Entity, Building>` pending map so each building is written once.
- **Storage acceptance (W5).** Two carts arriving at one full store in one tick.
  Applied in ascending `(Cell, Entity)` of the cart, clamped per arrival.
- **Draw order.** Unchanged: `x + y` then `x`, which is screen depth and is total.
  Any new drawable joins that sort rather than adding a second one.
- **Randomness.** None added.
  When it arrives, it is a `SplitMix64Rng` derived per decision from `(SaveGame::seed, tick, Cell)` rather than one shared advancing stream, because a shared stream makes the answer depend on which system drew first -- which is the very coupling this section exists to remove.

### Testing without a window, and the coverage gate

CI requires 100% line, function and branch coverage on the GNU leg, and the game's test target links `antwika::gfx::tests::mocks` rather than any backend.
Every new type in this increment is one of four shapes, and each has an established windowless pattern already in the tree:

- **A table or a pure function** (`kHousingRequirements`, `ratingsOf`, `serviceConferredBy`, `nearestAccepting`) -- a plain value test, exhaustive over the table where the table *is* the feature, as `WalkingTest` does over all 64 inputs.
- **A component** -- no logic, covered by whatever reads it, and by `ValueEqualityTest` for its `operator==`.
- **An `ecs::ISystem`** -- driven directly with a `World` and a `SystemScheduler` in the test, exactly as `BuildingSystemTest`, `SpawnSystemTest` and `WalkerSystemTest` already do.
  No renderer, no window, no clock.
- **Render-side work** -- asserted call-by-call against `gfx::mocks::MockRenderer`, and any widget lookup goes through `ui::Frame::rects` and `tests/WidgetPixel.hpp`, never a per-pixel sweep re-running `describe()`.

Two further obligations.
Read `docs/confirming-unreachable-branches.md` before writing any `GCOVR_EXCL_LINE`; the existing exclusions in `SaveGame.cpp` and `SaveMigration.cpp` are the worked examples of what a justified one looks like.
And a wide soak -- a thousand-tick city, if one is wanted -- belongs in an optimised build, not the `-O0` coverage build.

**The one new cross-cutting test to write, and it is the increment's most valuable:**
`AllocationOrderTest` builds the same city twice with the buildings created in two different orders, runs both for the same number of ticks, and asserts identical `GameSummary`s.
That is the test that would catch a system quietly depending on `ecs::View`'s order, and there is nothing in the tree today that would.

Alongside it, `ReplayDeterminismTest` gains a scenario long enough that coverage lapses, a house evolves, labour is contended and a cart completes a round trip -- because a determinism test over a city that did nothing agrees for the wrong reason.

## 7. What is deliberately not in this increment

- **House merging into blocks.** The one housing rule that needs a total order over neighbours; it would double W3.
- **Immigrant and labour-seeker walkers.** Caesar uses walkers for both; round 1 uses a house-local countdown and a city-wide allocation instead, so the increment adds routed walkers in one place (W5) rather than three.
- **Treasury, taxes, wages and the tax collector.**
- **The emperor, requests, and the four Caesar ratings as *targets*.**
  Note for whoever does this: a request is a function of the persisted seed and the tick, so it is **regenerated, not evented** -- and accepting one is a click.
  That is the single most tempting place in this whole design to add an event that must not exist.
- **Entertainment, religion and education as services**, and the buildings behind them.
- **Trade with the other three world-map cities**, and imports/exports through storage.
- **Fire, collapse and disease as consequences**, and rebuilding after them.
- **Farm output varying with terrain or fertility.**
- **Roadblocks, gatehouses and any control over where a walker may go**, beyond where roads are.
- **Advisor panels.** Round 1's only judgement of the city is two labels on the toolbar.
- **Any performance work on `stepTowards()`.**
  It re-runs a full A* per walker per step, and round 1 makes that worse by adding routed walkers.
  It is a known debt, it is not this increment's, and nothing here should make a cached route or a flow field harder to add.

### The order the later increments should come in

1. **Round 2 -- walkers for people.**
   Immigrant and labour-seeker walkers, migration at the map edge, unemployment.
   This is the increment that makes labour visible rather than arithmetic.
2. **Round 3 -- the empire.**
   Treasury, taxes, the four ratings as targets, and the emperor's requests regenerated from seed and tick.
   Depends on round 1's ratings and round 2's population being real.
3. **Round 4 -- the rest of the services.**
   Entertainment, religion, education, health extended, with housing tiers deepened to demand them.
   Cheap once round 1's coverage model exists -- mostly tables and art.
4. **Round 5 -- trade and the world map.**
   Routes between the four cities, imports and exports through storage.
   The first increment where the world map does something.
5. **Round 6 -- hazards and consequence.**
   Fire, collapse and disease as functions of lapsed coverage, and the first place randomness genuinely arrives -- under the derived-seed rule section 3 fixes now, so that increment does not have to invent it.
6. **Round 7 -- performance.**
   Replace the per-step A* with a cached route or a flow field, once walker counts justify it and not before.
