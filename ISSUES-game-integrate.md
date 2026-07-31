# game-integrate

## The city grid is per city, but the things standing on it are not

**Task:** step 1, wiring the world map into the running app.

**Blocker:** `WorldMapState` keeps a `PathIndex` and a `Camera` per city, and those now work: leaving a city and coming back shows the roads and the view it was left with, because the live pair is swapped in and out at each transition.
Walkers and buildings do not, because they are entities in one `ecs::World` and nothing says which city an entity belongs to.
Build a house in city 1, press M, open city 2, and the house is still standing there.

Making them per city needs a `City` component on every grid entity plus a filter in `snapshotOf()` and `WalkerSystem`, and a decision about whether a city you are not looking at keeps simulating.
That is a bigger change than wiring, and it also collides with the save format: `SaveGame` carries one `paths` and one `walkers` list, so a per-city session is a schema bump as well.

**Question for the human:** should a session be one grid that four cities take turns showing (what ships today, minus the leak), or four independent cities with four independent populations and a save format that carries all of them?

**What I did instead:** the roads and the camera are per city and correct; the entities are shared and documented as such in `WorldMapState.hpp`.
Nothing is silently wrong -- it is written down where somebody reading the class will see it.

## Loading a save inside a run reads a file inside the tick path

**Task:** step 2, the save/load screen.

**Blocker:** the *list* of saves is read once at startup and held in `SaveLoadState`, so it cannot differ between a live run and a replay -- that part is solved, and `listSaveGames()` says why at length.
A save's *contents* cannot be handled the same way: the Load button has to read the named file when the click asking for it arrives.
A replay of a session that loaded `town` therefore reproduces it exactly as long as `town.save.json` still holds what it held.
Overwrite that file between the recording and the replay and the two runs diverge.

This is inherent to a load button rather than something the design gave away.
The alternatives are all worse or bigger: hashing the file into the recording would make a replay fail loudly instead of quietly, and inlining the whole save into the recording would make `--record` files carry every session anybody ever loaded.

**Question for the human:** is "a replay is only as good as the files it names" acceptable, or should a recorded load carry a checksum so a changed file fails the replay rather than changing it?

**What I did instead:** shipped the load, and wrote the limitation into `SaveLoadSink`'s header so nobody reads it as a bug.

## A save carries no buildings

**Task:** step 2 and step 3.

**Blocker:** `SaveGame` has `paths` and `walkers` and no buildings, and its own header says buildings "belong beside this as their own member and their own schema property" whenever they exist.
They exist now.
Adding them is a save-format version bump and a migration, which the format is already built for (`standardSaveMigrations()` is an empty chain waiting for its first entry).

**Question for the human:** none, really -- this is a follow-up rather than a decision.
Flagging it so that "I saved, I loaded, my houses are gone" is not a surprise.

**What I did instead:** `GameSummary` and `printSummary()` do report buildings now, so a finished run says what was built even though a save cannot yet carry it.
