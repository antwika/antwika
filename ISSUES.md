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

Questions from the twelve agents run against the task list of 2026-07-31.
None of them blocked the work: each was implemented the conservative way, and the
question is only whether that choice should stand.
This section was written while the run was still in progress, so more entries may
follow.

## From the CLI extraction (`feat/cli-library`, merged)

### R1. Are the `antwika::replay` re-export headers transitional or permanent?

`antwika/replay/CommandLine.hpp`, `FlagSpec.hpp` and `CommandLineError.hpp` were
left behind as `using antwika::cli::...;` declarations, so `game::SaveCli` and
`poker::WatchOptions` compile untouched.
That is what kept the extraction from colliding with the four agents editing the
apps at the same time.
They are commented as transitional, and the follow-up is to migrate those two
callers and then delete the headers.
Confirm that, or say they should stay as a permanent alias surface.

### R2. Should `antwika::cli` support positional arguments?

It is flags-only, which is what every existing caller needs.
`apps/sound_demo` takes a bare filename rather than a flag, so it is the one CLI
that cannot migrate until the library grows positionals.
Leaving it means one app keeps parsing its own argument.

## From the RNG extraction (`feat/rng-library`, merged)

### R3. Should `antwika::rng` grow the positional hash `IDEAS.md` asks for?

`IDEAS.md` asked for `hash(seed, x, y) -> value` alongside lifting the generator
out.
It was not built: no call site in the repository needs randomness as a function of
position, since every one draws in a fixed order from a fixed seed.
The `IDEAS.md` entry was narrowed to say the hash is still outstanding rather than
deleted.
Say whether you want it built now or left waiting for a first customer.

### R4. `FakeRng` now lives in `antwika::rng`'s own `tests/fakes/`

It moved with the interface it doubles, which means `antwika_holdem_tests` links
another module's fakes.
That is already the norm in this repository, but it is a judgement call worth
confirming rather than discovering later.
