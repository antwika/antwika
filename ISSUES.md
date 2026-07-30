# Issues not addressed

Everything below was in scope and did not land inside the one-hour window.
None of it is broken work: the branch builds clean and the full suite passes.
Items that stopped because a decision is yours rather than an agent's are
cross-referenced to [`ISSUES2.md`](ISSUES2.md) instead of being argued twice.

## 1. The game does not yet read its labels from `antwika::i18n`

**Task:** *"Make the antwika game utilize this library to display labels in
either swedish or english."*

Half done.
`antwika::i18n` exists, is fully tested, and already contains every string
the game needs in both locales -- the four menu entries, the language label
and names, and the toolbar's own labels pinned to what `Toolbar.cpp` uses
today.
The game has a working language selector and shows Swedish or English.

What is missing is the join: `game::MenuLabels`/`labelsFor()` still holds its
own literals rather than reading the catalogue.
It was built as the single swap point precisely so this would be a small
change.

It stopped on a real conflict rather than on time -- see
[`ISSUES2.md`](ISSUES2.md) §3.
The catalogue spells Swedish with diacritics; the game transliterates,
because `gfx`'s font is ASCII-only and `textSize()` counts bytes.
Swapping as written would fail a test and ship a misaligned menu.

## 2. The game view is not re-centred after a window resize

**Task:** *"Make sure that the gfx window can be resized.
The antwika game view should still be centered after a window resize."*

The window half is done and merged: windows can be created resizable under
all three backends, and `IWindow::configuredSize()` now names the size the
app asked for apart from the size the window reports.

The centring is not done.
The shape is settled -- the halved difference between reported and
configured size becomes a `gfx::Point` offset applied at `GridScene::draw()`'s
destination rectangles, with nothing in the camera, `GridSink` or
`SceneSnapshot` touched.

It was deliberately not rushed, because the obvious version of it breaks
replay: see [`ISSUES2.md`](ISSUES2.md) §6.

## 3. *Save replay* reports an intent that nothing services

**Task:** the main menu's third entry.

The menu entry exists, is drawn, is clickable and reports its activation.
No file is written, because `app::RecordedRun::replayRecorder` is typed as
`ITickEventSink&`, so `main.cpp` cannot reach `TickEventRecorder::getEvents()`
mid-session.

Widening `RecordedRun` is the clean fix and touches every app, which is why
no agent did it unilaterally.
See [`ISSUES2.md`](ISSUES2.md) §2.

## 4. *Play game* does not restart the world

**Task:** the main menu's first entry.

It reports the intent, sets `gameBegun` and closes the menu.
Nothing resets the world, because a mid-run restart would put two sessions in
one recording with no marker between them.
See [`ISSUES2.md`](ISSUES2.md) §1 for the two ways out.

## 5. Coverage was measured on the merged branch, not by every agent

**Task:** the standing 100% line/function/branch rule.

Six of the eleven agents ran the `conan-coverage` build themselves and
reported 100% on their own code.
Three ran out of budget and argued coverage from their tests instead of
measuring it: `wire-menu`, `wire-path` and `wire-anim`, all Phase 2.

The specific lines they flagged as unmeasured are `walkHome()`'s three
branches, `RoadGraph::neighbours()`'s two, `headingTo()`'s four, the
`menuState` ternary's two arms, and `walkerLift()`'s `std::max` clamp.

The orchestrator ran the CI-equivalent gate once on the merged branch.
**It does not pass.**

Measured, with CI's own exclusions (`.*/tests/.*`,
`.*/apps/[^/]+/src/main\.cpp`, throw and unreachable branches):

- lines **100.0%** (5646 / 5646)
- functions **99.7%** (1100 / 1103)
- branches **99.7%** (3841 / 3852)

One of the four gaps is fixed: `Walker::operator==` is defaulted and gained
an `origin` field that no test varied, so a field-by-field branch was never
taken.
`WalkerTest.EqualityComparesEveryOtherFieldIndependently` now varies each
field on its own.

Three remain, and each needs more than a few minutes:

- `src/apps/game/src/GridScene.cpp` — one branch at the
  `animation::DirectionalClipSet` construction in `walkerClips()`.
- `src/libs/ecs/include/antwika/ecs/View.hpp` — functions and branches, and
  `World.hpp` likewise.
  These are **template instantiations**, and they were at 100% before this
  work.
  The new `View<Building>` and `View<Walker, Cell>` uses instantiate paths
  the existing tests never reach for those types, so the gap is a missing
  test per new instantiation rather than a defect.

The `main.cpp` composition roots are excluded by CI and read as 0% under a
plain `gcovr` invocation; use the workflow's exact filter list from
`.github/workflows/build.yml` before concluding anything is uncovered.

## 6. Cosmetic follow-ups nobody owned

- `docs/` gained four new normative notes (`pathfinding.md`, `animation.md`,
  `i18n.md`, `resizable-windows.md`) and `CLAUDE.md` now describes all four.
  Nothing moved to `docs/history/`, which is correct -- none of this has
  shipped and been superseded yet.
- No blog post was written for any of the three new libraries, though each
  has a design note that would carry one.
  The existing posts are written after the fact, so this is not overdue.
- The `sdl3` and `raylib` configurations were exercised by the `gfxresize`
  agent only.
  Every later branch was built and tested against the default `null` backend
  alone, so a backend-specific break in the game's new drawing code would not
  have been caught.
