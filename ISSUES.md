# Issues needing your input

Everything asked for was built.
This is what is left for you to decide, and what you should know before reviewing.

The branch is `agents/integration`.

An additional round of parallel work has since been merged to
`integration/parallel-tasks`; its open questions are appended at the bottom of this
file under "Round two".
The entries in this first half are from the earlier session and are unchanged,
except where round two closed one, which is noted in place.

---

## Answered on 2026-08-01

Every question below is annotated in place with the answer; this is the
index.
Three were implemented in the same pass and are on `fix/texture-scale-mode`;
the rest are decided and not yet built.

| # | Answer | State |
| --- | --- | --- |
| R2 | Nearest-neighbour, pinned in both backends, no API change | **done** |
| R6 | Retire the three transitional `antwika::replay` headers | **done** |
| R7 | No positional arguments; `apps/sound_demo` names `--file` | **done** |
| R1 | A save carries the live city, not the whole session | no change |
| R4 | The placement border draws for the road tool too | no change |
| R8 | No positional hash in `antwika::rng` | no change |
| R3 | Rename `ITickSource` to `ITickEventSource` | to build |
| R5 | Cancelling build mode reaches a real "nothing selected" | to build |
| R9 | Over-feeding the companion annoys it | to build |
| R9b | Tick counter keeps running; a city is entered **paused**; the FPS readout shows a placeholder for its first second | to build |
| R10 | Rename `antwika::ttf` to `antwika::font` | to build |

Still unanswered: entries 1, 3, 4 and 5 below, the load-checksum question
in `ISSUES-game-integrate.md`, and four of R10's five sub-items.

R5 and R9b are the two with a cost worth knowing before they are picked
up.
R5 changes what the already-recorded right clicks in
`src/apps/game/replays/demo.json` mean, so that demo replay probably has
to be re-recorded.
R9b's "enter a city paused" is a new behaviour rather than a default
being changed, and it is simulation state, so it lands in the tick path
with the rest of `PauseState`.

---

## Still open — your call

### 1. A face-down hole card now shows its rank

`apps/poker` drew its card art and its card labels from two independently computed layouts, which disagreed by enough that a card's rank and suit landed on the *neighbouring* card.
That is fixed: both now come from one layout.

The side effect is that two behaviours which always disagreed are now visible together.
Before showdown the art draws a card *back* while the label still prints the rank and suit — so a face-down card reads as a back with `As` written on it.
Both predate the fix; they were simply far apart on screen.

Choosing means either hiding the label until showdown (a spectator app arguably wants that information) or always drawing the face (which retires `kCardBackSlot`).
Left as it was rather than decided for you.

### 2. Per-city entities in `apps/game` -- fixed, with one thing left for you

Walkers and buildings are per city now, exactly as the roads and the camera already were.
A city keeps its contents as a `game::CityGrid` value while it is put away, and `WorldMapState` destroys and recreates the entities on the one live `World` as cities are swapped -- so no city tag on an entity and no filter in any system was needed, and no save-format bump was either.

The decision that had been left open is therefore: **a city is a grid, and what stands on it belongs to it.**
The four are independent, and a city nobody is looking at neither runs nor shows anywhere else.

**Answered 2026-08-01: a save carries the live city, and that is what a
save is.**
No version 3 and no migration; every existing version-2 file keeps
loading, and "save, load, and the three cities you were not in are
empty" is the accepted behaviour rather than a gap.
The original text follows.

**What is left for you:** a save still carries *one* grid -- the live city's -- which is what version 2 has always meant, and it is why no bump was needed.
It is also what a save has always done with the roads and the camera, so the file is consistent rather than newly lossy.
But it does mean "save, load, and the three cities you were not in are empty".
Making a save carry the whole session is a version 3 with a migration that reads a version 2 document as the one city it was written from -- straightforward, and a decision about what a *save* is rather than what a city is, so it was not made unilaterally either.

### 3. Demolition

Nothing in `apps/game` can remove a building.
This surfaced as a coverage gap — `ComponentStorage<Building>::remove` was dead code — and was closed by making loading destroy the old city, but that is a workaround.
A demolish tool is probably the real gap.
Not built, because it is a new mechanic rather than a fix.

### 4. 3D has no caller

`gfx::IRenderer3D` is implemented by `null` and `raylib`, with `apps/gfx3d_demo` proving it renders.
`sdl3` reports no 3D renderer, which is a conforming answer rather than a gap.

But nothing in the repo *wants* 3D yet.
The interface is speculative until an app needs it, and it was built additively so it costs nothing while it waits.
Say if you want a real use, or want it withdrawn.

### 5. Which key returns from a city to the world map

`Escape` was already spent on quitting, so `M` was chosen.
Decided, not blocked — but easy to change and worth your eye.

---

## Decisions taken on your behalf

Each was a judgement call, is reversible, and the reasoning is in the commit body.

- **A player with chips in the pot may not leave a hand, folded or not.**
  A stake is committed to the pot rather than lent to it, and folding forfeits the claim without withdrawing the chips — which is also what a card room does.
- **Invalid blinds are refused at the `Table` constructor** rather than clamped by a saturating subtraction.
  A clamp would leave the pot exactly as wrong while replacing an absurd number with a plausible one, which is harder to find.
- **The save format adopted `"version"`**, matching replay documents, rather than its own `"schemaVersion"`.
  Replay files with `"version"` already exist on disk; no save file did, so the format with no users paid the cost.
- **Buildings spawn a walker every 20 ticks** onto the lowest-ordered adjacent road, from a per-building countdown.
  Towers spawn nobody.
  A building with no road holds its countdown at zero rather than banking one, so laying a road releases one walker and not a queue.
- **The save-file list is read once at startup**, not inside the tick path.
  Re-listing a directory mid-run would not replay.

---

## What you should know before reviewing

### The coverage gate has one documented exclusion

`View.hpp`'s two lambdas carry `GCOVR_EXCL_LINE` for a reason that is new to this repo, and is written up as case (d) in `docs/confirming-unreachable-branches.md`.

gcov emits one function record per template instantiation.
Where a caller inlines the body, the shared out-of-line copy is never called, so its record reads zero — while the line counters, attributed through the inline expansion, show the code running.
Three such records were at zero with every line inside them covered.

The exclusion is coarser than the others in this file: the marker is per *source* line and every instantiation shares one, so it drops the covered records too.
That is stated in the doc rather than hidden.
`gcovr --merge-mode-functions=merge-use-line-max` looks like the right fix and is not — the records are distinct symbols, not copies gcovr will fold.

### A test flake was found and fixed, and I cannot prove it is gone

Two `apps/game` save/load tests failed only under full-suite parallel load.
Two causes, both real:

1. CTest registers every case as its own process, and the fixtures named their scratch directory after the *fixture* — so concurrent cases deleted each other's files.
2. The name was then the same on every run, so a fixture that removes the directory and immediately recreates it races the filesystem retiring and reissuing one entry.

Both are fixed (per-case naming, plus the process id).
The suite has been green for six consecutive full runs since.
But the original failure rate was roughly one run in four, so six green runs is evidence, not proof.
If it recurs, the fixtures are `SaveLoadSinkTest` and `SaveDirectoryTest` and the helper is `src/apps/game/tests/ScratchDirectory.hpp`.

### Two tests are slow by construction

`SaveLoadSinkTest`'s `pixelOn()` finds a widget by scanning 163,840 canvas positions and calling `describe()` at each.
That is 4-9 seconds per case under coverage instrumentation.
It is correct and it is why those cases are under load long enough to have exposed the bug above.
`ui::Frame::rects` — added this session — now makes a widget's rectangle directly askable, so that helper could become a lookup.
Not done, because the tests belonged to another agent's lane while it was still running.

---

# Round two

Thirteen agents worked the task list of 2026-07-31: twelve in parallel, then one
more for the library split once their work was merged.
Everything asked for was built, and the questions below are the ones the agents
declined to answer for you.
None of them blocked any work -- each was implemented the conservative way, and the
question is only whether that choice should stand.

All of it is on `integration/parallel-tasks`: 39 commits, 2942 tests passing, all
three checker scripts green, every new module reporting 100% line, function and
branch coverage under CI's own gcovr flags.

## Decisions the agents declined to make

### R1. Should a save carry the whole session, or only the live city?

This is the one that came out of your world-map bug, and it is the most consequential
question in this file.

Buildings and walkers leaked across cities because `WorldMapState` swapped a
`PathIndex` and a `Camera` per city while the entities standing on the grid lived in
one shared `ecs::World` with nothing saying which city they belonged to.
That is fixed: a closed city is now stored as a `CityGrid` of plain values.

**Answered 2026-08-01: the live city only.**
The conservative choice stands and there is nothing to build.

The save format was **not** bumped, because a save carries one grid -- the live
city's -- exactly as it already did for the roads and the camera.
So every existing version-2 file still loads.
But it does mean that saving, loading and then walking to another city finds it
empty.
Making a save carry all four cities is a version 3 plus a migration reading a v2
document as the one city it was written from, and it is a decision about what a
*save* is rather than a bug.

### R2. Which of the two backends is right about texture scaling?

**Answered 2026-08-01: nearest, and it is done.**
`IRenderer::createTexture()` now says a texture is sampled
nearest-neighbour, SDL3 sets `SDL_SCALEMODE_NEAREST`, and raylib sets
`TEXTURE_FILTER_POINT` rather than inheriting the default it already had.
`Sdl3RendererTest.DrawTexture_ScalesWithoutSmoothing` reads the seam
pixel back through SDL: nearest gives 255, and linear gave 135.
The original text follows.

Found while fixing the SDL3 transparency bug, and left deliberately unfixed because
it is an aesthetic call rather than a defect.

SDL3 defaults a texture to `SDL_SCALEMODE_LINEAR`; raylib's `rlLoadTexture` sets
`GL_NEAREST`; nothing in this repository sets either.
So a scaled blit is smoothed under one backend and crisp under the other -- measured
at 667 distinct colours against 105 on the same `antwika_gfx_demo` frame.
It matters most for `apps/game` and `apps/poker`, whose pixel-art atlases are scaled
to whole tile sizes.
Nearest is almost certainly what pixel art wants and would make the two backends
agree, but linear is defensible for art that is not pixel art.
It is a one-line addition to `createTexture()` plus a line in `IRenderer`'s contract.

### R3. Is `ITickSource` the name you want?

**Answered 2026-08-01: rename it to `ITickEventSource`.**
Accuracy wins over line length.
Not built yet; it is one rename across the seam's implementers.

`antwika::replay` was split rather than renamed: `antwika::simulation` owns the loop
(`EngineLoop`, `TickPacer`, `WindowInputSource`, and the seam) and `antwika::replay`
owns the recording and its format, depending on it.
The reasoning is in `wiki/libraries/simulation.md`.

`IReplaySource` was renamed `ITickSource` as part of that, since seven of its
implementers never touch a recording.
`ITickEventSource` is more literally accurate -- it supplies a tick's *events* -- and
was passed over for line length.

### R4. Should the build-placement border draw for the road tool too?

**Answered 2026-08-01: yes, it draws for the road tool too.**
Nothing to change.

It does today, on the grounds that a road is a 1x1 footprint and that a border
appearing for five tools and not the sixth reads as a bug.
Your request said "when placing a new building", so if roads should be bare it is a
one-line change in `drawGhost()`.

### R5. Should cancelling build mode reach a genuine "nothing selected" state?

**Answered 2026-08-01: reach a real "nothing selected" state.**
So `UiOverlay::tool()` becomes an optional.
Worth knowing before starting: this changes what the recorded right
clicks in `src/apps/game/replays/demo.json` mean, so that demo replay
probably has to be re-recorded.

Right-click currently leaves build mode by falling back to the road tool, because
right-click with the road tool selected already means "drop a walker" and the
recorded right-clicks in `src/apps/game/replays/demo.json` still have to mean that.
A true "no tool selected" state is the classic city-builder behaviour and is what a
future reader will probably expect, but it needs `UiOverlay::tool()` to become an
optional and it changes what already-recorded left clicks mean.

### R6. Are the `antwika::cli` transitional headers permanent?

**Answered 2026-08-01: retired, and it is done.**
All eight callers across `game`, `poker` and `atlas_editor` now name
`antwika::cli` and link it directly, and the three headers are deleted.
The two answers agree, as this entry said they probably should.
The original text follows.

`antwika/replay/CommandLine.hpp`, `FlagSpec.hpp` and `CommandLineError.hpp` are
`using` re-exports left behind so that `game::SaveCli` and `poker::WatchOptions`
compiled untouched while four other agents were editing those apps.
They are commented as transitional and still have roughly eight callers.
Retiring them means migrating those callers and deleting the three headers.

Note that the later library split deliberately left **no** such shims, on the
grounds that it moved all 113 of its call sites in the same commit, so shims would
have been dead on arrival.
The two answers should probably agree.

### R7. Should `antwika::cli` support positional arguments?

**Answered 2026-08-01: no positionals, and it is done.**
A positional cannot be described in the `FlagSpec` table, so the property
the library is built on -- that the parse and the help text come off one
list -- would hold for flags and quietly not for the rest.
`apps/sound_demo` names `--file <path>` instead, so nothing in the tree
reads its own `argv` any more; `docs/cli.md` records the rule.
The original text follows.

It is flags-only, which is what every caller needs.
`apps/sound_demo` takes a bare filename rather than a flag, so it is the one CLI that
cannot migrate onto the library until positionals exist.

### R8. Should `antwika::rng` grow the positional hash `IDEAS.md` asks for?

**Answered 2026-08-01: leave it unbuilt.**
The narrowed `IDEAS.md` entry stands.

`hash(seed, x, y) -> value` was not built: no call site needs randomness as a
function of position, since every one draws in a fixed order from a fixed seed.
The `IDEAS.md` entry was narrowed rather than deleted.

### R9. Should over-feeding the companion annoy it?

**Answered 2026-08-01: yes, over-feeding should annoy it.**
A third violation, in the remaining arm of `Pet::tap()`.
Not built yet.

A tap while the companion is awake and not hungry currently does nothing, which is
what stops tap-spamming being a strategy.
Your specification named only two violations -- leaving it hungry, and tapping while
it sleeps -- so harmlessness was the conservative reading.
Making it a third violation is one arm of `Pet::tap()`.

### R9b. Three choices in the pause and the counters

**Answered 2026-08-01, all three:**

- The tick counter **keeps running**, as it does today -- it shows the
  engine tick, and that is the point of a `life`-style pause.
- A city is **entered paused** from the world map, rather than a pause
  surviving one.
  Progress is then something somebody asks for, which is a new behaviour
  rather than a default flipped, and it is simulation state like the
  rest of `PauseState`.
- The FPS readout shows a **placeholder** for its first second rather
  than `0`.

Not built yet.
The original text follows.

- **The tick counter does not freeze while paused.**
  It shows the engine tick, which by design keeps advancing -- that is the whole
  point of a `life`-style pause, where the tick, the commit and every observer go on
  running and only the simulation systems stop.
  A "ticks simulated" counter instead is a one-line change in `UiSink`.
- **A pause survives leaving the city.**
  Pausing, going to the world map and coming back leaves the run paused.
  Simple and defensible, but nobody stated it either way.
- **The FPS readout shows 0 for the first second**, and only over the city screen.
  A "--" placeholder, or a readout in every mode, are both easy.

Worth knowing about the pause itself: it is a *build* pause rather than a freeze.
`WalkerSystem`, `BuildingSystem` and `SpawnSystem` stop; the camera, the toolbar and
placement keep working, so a paused city can still be panned over and built on.

### R10. Smaller calls, grouped

- **The companion reaches `TickPacer` through an adapter holding an empty
  `ecs::World`**, because `TickPacer` is an `ecs::ISystem` and the app keeps no
  world.
  Judged better than a third copy of a class the project has already deduplicated
  twice; moving the sleep into `RenderSink` is a two-line change if the empty world
  reads as a smell.
- **`FakeRng` moved into `antwika::rng`'s own `tests/fakes/`**, so
  `antwika_holdem_tests` now links another module's fakes.
  Already the norm here, but worth confirming.
- **The atlas editor's palette is twelve compiled-in colours.**
  An artist may want different ones, or a hex-entry field instead.
- **Rename `antwika::ttf` to `antwika::font`** (answered 2026-08-01), so
  the name does not promise one format when a second outline path is a
  plausible follow-up.
  It touches the directory, the namespace, the CMake target, every
  include of it, `docs/ttf.md`, `wiki/libraries/ttf.md` and `CLAUDE.md`
  -- wide but mechanical.
  Refusing CFF is unchanged by it; the name simply stops claiming
  otherwise.
  The four sub-items around it are **still unanswered**.
- **`antwika::ttf` refuses OpenType/CFF fonts and font collections by name**, though
  stb could read CFF; supporting it means a second outline path and a second
  synthetic fixture.
  Its `GlyphAtlas::Options` defaults (512-pixel maximum width, 1-pixel padding) are
  guesses at what a first caller wants.
- **The hover readout in `apps/game` lists only the resources its bars gauge**, so a
  *source* building shows just its name rather than the inert stock it holds --
  chosen so the panel and the bars never tell two stories.
  A carrying walker shows its bar even when empty, since "this food walker is spent"
  is worth seeing.

## Known gaps, no decision needed

These are simply not done, and are recorded so they are not discovered later.

- **`README.md` lists the project's applications and omits all four new ones**
  (`ui_demo`, `companion`, `atlas_editor`, and the `gfx3d_demo` line's neighbours).
  Every agent was scoped away from that file so twelve of them would not collide in
  it, which is exactly why nobody added their line.
  It needs one line each.
- **Coverage over the final merged tree** was measured after everything landed,
  rather than trusted from the per-agent runs.
  Each agent measured its own module at 100% before merging, but two things made a
  combined run worth having: the library split moved files between modules, and the
  toolbar/HUD agent's own coverage run was interrupted before it reported.
  That run found two gaps and both are now closed, so the gate reports 100%
  lines, functions and branches over the merged tree.
  One of them, three branches in `src/libs/ecs/include/antwika/ecs/View.hpp`,
  **predates this work**: the file is byte-identical to `main`, and the cause is
  `gcovr --exclude-throw-branches` failing to strip three allocation-failure edges
  that raw `gcov -b` tags `(throw)`.
  So `main` very probably fails the gate today for the same reason.
  That was inferred rather than proven -- no coverage build of `main` was run.
- **Only the `gcc-linux-x86_64` profile was built.**
  The LLVM and MinGW legs are untested locally; CI covers them.
- **`apps/game` does not yet use the new `antwika::ui` hover pass.**
  Adopting it is one `applyHover()` call in `main.cpp` -- `Toolbar` and `UiSink` need
  no change at all -- and it would retire the limitation `CLAUDE.md` still documents,
  that a button there lights up on the press rather than on approach.
  It was deferred only because three agents were editing that app at the time.
- **`ISSUES-game-integrate.md`'s third section is stale.**
  It says a save carries no buildings; the format is version 2 and has carried them
  for a while.
