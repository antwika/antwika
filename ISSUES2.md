# Issues needing your input

These are decisions the agents deliberately did not take on your behalf.
Each one was reached by implementing the most defensible reading and then
writing down what the alternative would cost, so nothing here is blocking a
build -- the branch is green either way.

Ordered by how much a wrong answer would cost to undo.

## 1. What *Play game* does to a session already in progress

**Task:** the main menu.

`replay::EngineLoop` runs one monotonic tick sequence, and the recorder
appends to a single file.
Restarting the world mid-run would leave a recording holding two sessions
with no marker between them, and `ReplayReader` would replay that as one
continuous session, which is a silent corruption rather than an error.

The menu currently reports the intent, sets `gameBegun` and closes; nothing
resets the world.

Pick one:

- **Drop the entry once a game has begun**, exactly as `entriesFor()`
  already drops *Resume game* before one has.
  Cheapest, and it makes the menu honest about what it can do.
- **Make it mean "abandon this session and start another"**, which needs a
  recording story: either the run ends and a new file is opened, or the
  format grows a session marker `ReplayReader` can split on.

## 2. *Save replay* when the run has no `--record` path

**Task:** the main menu.

Nothing in this project can open a file dialog, so the menu cannot ask where
to write.
The entry reports an intent that the app services using the path `--record`
already gave it; a run started without `--record` has nowhere to put it.

Two further wrinkles found while wiring it up:

- `app::RecordedRun::replayRecorder` is typed as `ITickEventSink&`, so
  `main.cpp` cannot reach `TickEventRecorder::getEvents()` to write a file
  mid-session.
  Widening `RecordedRun` is the clean fix -- every app already passes that
  recorder straight through -- and a tee sink plus a small `ISystem`
  observer is the alternative.
  Until one of them lands, *Save replay* reports its intent and nothing
  writes a file.
- The shipped `--record` epilogue writes the file when the run *ends*, so a
  mid-session save is either a second partial file or an early flush of the
  only one.

Pick one: grey the entry out when there is no path (`ui::ButtonSpec::state`
already takes an appearance override, and `MenuState` needs one more field),
or give it a default path, or leave it reporting an intent nobody services.

## 3. Swedish cannot currently be spelled correctly

**Task:** the i18n library and the language selector.

`antwika::gfx`'s bitmap font covers ASCII only, and `gfx::textSize()`
measures **bytes**.
A label containing `å` or `ä` therefore measures two cells wider than it
draws, leaving the button wider than its text.

The game's Swedish labels are consequently transliterated -- *Ateruppta
spel*, *Sprak* -- and `MenuLabelsTest` locks that in so removing it has to be
deliberate.
The `antwika::i18n` catalogue, written independently, spells them properly
with diacritics.

That mismatch is also why the i18n library is **not yet behind the game's
labels**: swapping `labelsFor()` over as written would fail that test and
ship a misaligned Swedish menu.
`MenuLabels` was built as the single swap point, so the change is small once
this is settled.

Pick one:

- **Teach `antwika::gfx` Latin-1** and make `textSize()` count code points
  rather than bytes.
  The real fix, and it is a `gfx` change, not a menu one.
- **Fold to ASCII inside `labelsFor()`** as an interim, keeping the
  catalogue correct and the rendering safe.
- **Keep the two spellings diverged**, which is the status quo and the worst
  of the three.

Two smaller mismatches to settle at the same time: the catalogue's
`MenuPlayGame` is *Spela* where the menu says *Spela spel*, and the
catalogue has no id for the menu title.

## 4. Should `IWindow::size()` be renamed `reportedSize()`?

**Task:** resizable windows.

The pair is now `configuredSize()` (what the app asked for, safe for layout
and hit-testing) and `size()` (what the window currently reports, which no
simulation may read).
The second name does not say out loud that it is a report.

The rename was not made because it touches `src/apps/gfx_demo`,
`src/apps/poker` and six app test files, all outside the agent's ownership.
It is a mechanical one-sweep change: two production call sites, one
`MOCK_METHOD`, three backend overrides, two app-owned fakes.

## 5. Should `configuredSize()` be a pure virtual?

**Task:** resizable windows.

`docs/STYLE_GUIDE.md` says interfaces are pure abstract, and this one member
is not -- it defaults to answering with `size()`.
Making it pure would have broken `FakeWindow` and `SpectatorWindow` in
`src/apps/poker/tests/`, outside the agent's ownership.

Adding the method to those two fakes is two lines each, after which the
interface member can be pure again.
All three real backends already override it, and `IWindowTest.cpp` pins the
default so it is not an untested line.

## 6. A resized window centres the view but not the clicks

**Task:** "the game view should still be centered after a window resize".

This is the one item where the obvious implementation would break replay, so
read this before finishing it.

Centring is a drawing concern and `docs/resizable-windows.md` explicitly
sanctions it: the offset belongs in `RenderSystem`, never in the camera,
`GridSink` or `SceneSnapshot`.
But hit-testing keeps using the **configured** size, by design -- that is
what makes a recorded click mean the same cell on a machine whose window is
a different size.

So on a resized window the grid is drawn centred while a click still resolves
against the un-offset layout, and lands on the pre-resize cell.
`UiOverlay` has the same property and must **not** be offset, or the toolbar
would light up somewhere other than where it was pressed.

The tempting fix -- feeding the offset back into hit-testing -- is exactly
the replay break the doc forbids.
The honest options are to let the window's reported size drive a *recorded*
resize event, or to letterbox rather than re-centre, or to accept the
mismatch and document it.
