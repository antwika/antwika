# Issues needing your input

Every task in the batch was attempted and merged.
These are the decisions the agents could not make for you, plus the follow-ups worth knowing about.
Each entry says what was blocked, the question, and what shipped instead so nothing is silently missing.

The branch to review is `agents/integration`.

---

## 1. There is no walker spawner to make "spawn more often" mean anything

**Task:** "It seems like the walkers are spawning a bit too infrequently, can you make them spawn more often?"

**Blocker:** `apps/game` has no automatic walker spawner and no spawn interval at all.
A walker exists only because somebody right-clicked a path tile — `game::GridSink::placeWalker()` is the single place a `Walker` component is ever added, and it runs off an `input.pointer_button_pressed` event.
There is no timer, no rate, no budget and no interval constant anywhere near walkers.
The only per-tick interval in the app is `kTickInterval` in `main.cpp`, which is `TickPacer`'s wall-clock pacing for the whole run — halving that would double the speed of everything, including the walking cadence the other half of this task just halved.

**Question:** which did you mean?

- (a) Add a new automatic spawner that drops a walker every N ticks — if so, onto which tile, and what N?
- (b) Make one right-click drop two walkers.
- (c) Halve `kTickInterval` so the whole simulation runs twice as fast.

**Shipped instead:** the other half of the task only.
Walkers now advance one cell every two ticks, via a per-walker integer countdown in the `Walker` component.
A tick-number modulus was rejected because it would make walkers dropped on different ticks march in lockstep.

---

## 2. The placement ghost cannot follow a freely moving pointer

**Task:** "Show a placeholder (with slight opacity) to indicate where a building would be placed."

**Blocker:** `apps/game` attaches `input::IdleMotionSource`, which deliberately withholds pointer movement while no button is held, so that a `--record` file does not fill up with positions nothing ever read.
The ghost is therefore restated only when a click, a wheel or a key arrives: it jumps to where the last click was rather than gliding under the cursor.
This is the trade `CLAUDE.md` already describes for the toolbar's hover appearance, now visible on the grid as well.

**Question:** is a live ghost worth what removing the gate costs a recording?

There is a middle answer: keep the gate, but let it release a latched movement once per tick even with no button held.
That costs one recorded movement per tick instead of one per reported pixel.

**Shipped instead:** the ghost as described, at `alpha = 110`, drawn from the same tile the real placement uses.
The limitation is documented in `BuildGhost`'s and `GridSink`'s headers so nobody reads it as a bug.

---

## 3. Should a finished run report what was built?

**Task:** follows from the build palette.

**Blocker:** `GameSummary` and `Game.cpp` were owned by a different agent this session, so `printSummary()` still reports only paths and walkers, not buildings.

**Question:** should a finished run print what was built, alongside "Paths laid" and "Walkers"?

**Shipped instead:** nothing.
The buildings are already in the `World` and in `SceneSnapshot`, so this is a two-line addition whenever you want it.

---

## 4. 3D rendering: how far did you actually want to go?

**Task:** "You may refactor the code base to support actual 3D rendering."

**What shipped:** GLM as a dependency, a render-side `Vec3`/`Mat4`/`Transform`/`Camera3D` math layer, and `IRenderer3D` as a sibling interface discovered through a non-pure `IRenderer::renderer3d()` that returns `nullptr` by default.
The `null` backend implements the whole thing.
The 2D API is untouched, which is why nothing else in the repo broke.

**Question:** no real backend draws 3D yet — `sdl3` and `raylib` inherit the `nullptr` default, because building raylib from source did not fit the time budget.
Do you want a real backend implementation next (raylib is much the easier of the two), and do you want an `apps/gfx3d_demo` to prove it?

There is a deeper question this task did not settle: nothing in the repo currently *wants* 3D.
The interface is speculative until an app needs it, and it was built additively precisely so that it costs nothing while it waits.

---

## 5. Three finished modules are not reachable from the running game

Not a design problem — a concurrency one.
`main.cpp`, `Game.cpp` and `RenderSystem` could only be owned by one agent at a time, and the main-menu agent had them.
So these are complete and tested, but not yet wired in:

- **World map.**
  Generation, city placement, layout, scene and click-sink are done and tested.
  The "World Map" entry on the main menu is an inert placeholder (a `ButtonSpec` with no id, so it cannot be hovered or activated rather than merely looking disabled).
  The agent left a five-step integration hook in `WorldMap.hpp`.
- **Save/load.**
  The versioned format, the migration chain, the file I/O and `--save`/`--load` flag parsing are done and tested.
  Nothing calls them yet, and the "Load Game" menu entry is the same kind of placeholder.
- **The text field and dropdown.**
  Both widgets are done, tested and at 100% coverage, but nothing in `apps/game` uses them yet — so the replay picker you asked for exists as capability, not as a screen.

**Question:** none — this is simply the top follow-up, and the natural next session.
Flagging it so you are not surprised that the world map passes tests but does not appear on screen.

---

## 6. Which key returns from a city map to the world map?

**Decided, not blocked — but worth your review.**
`Escape` is already spent on quitting, so the world-map agent chose `M`.
Say if you want something else.

---

## 7. Two conventions were changed to make things agree

Both were judgement calls made to remove an inconsistency, and both are cheap to reverse now and expensive later.

- **The save format adopted `"version"` as its schema-version key**, matching replay documents, rather than its own `"schemaVersion"`.
  Reasoning: replay files with `"version"` already exist on disk; no save file does yet, so the format with zero users paid the cost.
  Consequence: a replay and a save now look alike at the version member, and `"magic"` (`"antwika-game-save"`) is the only thing distinguishing them — it is checked.
- **`SchemaVersionError` narrows `ReplayFormatError`** rather than sitting beside it, so every existing `catch (ReplayFormatError &)` keeps working while a caller that wants to say "this file is from a newer release" can catch the narrower type.

---

## 8. Coverage is verified for some of the new code, not all of it

CI requires 100% line/function/branch coverage on the GNU leg.
Four agents ran a real coverage build and confirmed 100% on their code (`ecs_commons`, `libs/ui`, the `game` save files, `libs/replay`).
The rest wrote tests aimed at every branch but did not run `gcovr`, because a coverage build on a machine running many concurrent compiles was not affordable inside the time budget.

**Expect the coverage gate to be the first thing that fails in CI**, most likely in `apps/tower_defence` (`BattleScene.cpp`'s colour arms, `GridLayout.cpp`'s early returns) and `apps/game`'s world map.
This is a known gap, not a surprise.
