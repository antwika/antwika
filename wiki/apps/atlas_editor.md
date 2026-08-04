# apps/atlas_editor

`src/apps/atlas_editor/` — painting the sheet the game blits.

## What it is

`antwika_atlas_editor` is a pixel editor for the texture atlas `apps/game` draws itself from.
It exists because that sheet is hand-drawn art and is the source of truth for how the game looks -- see [`game-texture-atlas.md`](game-texture-atlas.md), which is the contract the art has to meet and which this editor does nothing to enforce.
What it gives an artist is the one thing an outside tool cannot: the slot boundaries the game addresses by, drawn over the sheet, so the tile being painted is the tile `game::TileAtlas.hpp` will blit.

It is an ordinary application of this codebase's tick loop rather than a tool bolted on beside one.
One tick is one frame, a click is the only input, `--record` and `--replay` work as they do everywhere else, and every pixel it puts down is regenerated from the recorded clicks rather than persisted.
**It defines one event of its own, and that event carries no pixels**: `atlas.opening_sheet` announces a fingerprint of the sheet a live session opened on, ahead of the recorder, on sudoku's `PuzzleSource` precedent.
The sheet decides what every later click means -- the Pick tool lifts a colour *off it* -- so a replay run checks the recorded announcement against the sheet it actually opened and refuses a mismatch loudly, instead of repainting different pixels in silence against a `--image` that changed between runs.
Everything else is `atlas_editor::EditorSink` turning a press into a painted pixel inside the tick path, so a `--record` file holds the click and a replay works out again which pixel it landed on and what colour went there.

## Opening a sheet

```sh
build/bin/antwika_atlas_editor/antwika_atlas_editor \
    --image src/apps/game/assets/atlas_1x1.png \
    --out my-atlas.png
```

`--image` is the PNG to open and `--out` is the PNG a save writes.
**They are deliberately separate, and neither defaults to the other.**
The sheet somebody is most likely to open first is the game's own, and one stray click on `save` should not be able to replace the art with a half-finished experiment.
Naming `--out` is how an artist says which file is theirs; without it the `save` button reports that there is nowhere to write to and changes nothing.

Given no `--image`, the session starts on a blank, fully transparent sheet of `--sheet <w>x<h>`, which defaults to the game's own 1x1 sheet at 512x768; the game's 2x2 and 3x3 sheets are opened with `--image` instead of started blank.
**That default is the game's contract rather than a taste**: `game::requireAtlasSize()` refuses any other size at startup, so a sheet started blank here and saved is one the game will only open if the two numbers agree.
`atlas_editor::kDefaultSheetSize` is therefore pinned to `game::atlasSizeOf(AtlasKind::OneByOne)` by `DefaultSheetSizeTest`, with a `static_assert` as well as a case, so a row added to that sheet is a red build here rather than a refusal somebody meets much later holding an afternoon's art.
That test is the only thing under `src/apps/atlas_editor/` that names `apps/game`, and the include reaching it belongs to the test target alone -- the editor itself builds knowing nothing about the game.

`--tile <w>x<h>` is what the grid overlay divides the sheet into, defaulting to the 1x1 sheet's 64x96 sprites; gridding the game's other two sheets is `--tile 96x112` and `--tile 128x128`.
It is a drawing aid and nothing else: no tool, no click and no saved byte depends on it, so a wrong `--tile` shows a misleading picture and cannot damage a sheet.
It is also what the sprite guides below are worked out from, so naming the right one is what puts the footprint diamonds where that sheet's sprites actually keep them.

The default `null` graphics backend opens no window and draws nothing, so **an editing session needs a real backend**:

```sh
conan install . -of build-sdl3 -o gfx_backend=sdl3 \
  -c tools.cmake.cmake_layout:build_folder_vars="['options.gfx_backend']" \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan-sdl3.lock

cmake --preset conan-gfx_backend_sdl3-release
cmake --build build-sdl3 -j24
build-sdl3/bin/antwika_atlas_editor/antwika_atlas_editor --image atlas.png --out mine.png
```

## What the mouse does

- **Left button**: whatever the selected tool is, on the pixel under the pointer, and on every pixel a drag crosses -- except under `SELECT`, whose left button is a gesture rather than a brush.
- **Right button**: drops the marked rectangle when there is one; and, under every tool but `SELECT`, erases, so taking a mistake back never costs a trip to the toolbar.
- **Middle button, dragged**: pans the sheet across the window, whichever tool is in hand.
- **Wheel**: zooms in and out, keeping the pixel under the pointer under the pointer.

**"Every pixel a drag crosses" is the whole segment rather than one dot per event**, and that is what the held-button arms actually walk.
A window system reports a fast stroke as a handful of long jumps, so painting only where each event landed left a dotted line with the gaps in between bare -- which is exactly what a fast hand produces and a slow one never does.
The walk is integer Bresenham from where the pointer was to where it is, for the no-floating-point reason below: which pixel a press means is simulation state, and a float's last bit is not the same on every toolchain.
Repainting the pixel the last event already put down costs nothing, since a write of the colour already there is no change at all.

The five tools are `PAINT`, which puts the selected colour down, `ERASE`, which makes a pixel fully transparent, `FILL`, which spreads the selected colour over every pixel joined to the one clicked that holds the colour that one does, `PICK`, which takes the colour under the pointer as the one to paint with, and `SELECT`, which is the one below.
A picked colour that no swatch offers leaves no swatch shown as chosen, rather than the nearest one lighting up and lying about it.

**`FILL` is a tool here and not a command, and nothing about it is persisted.**
It is four-connected, bounded by the sheet, and walked from an explicit stack rather than by recursion -- a fill over the game's own 1x1 sheet is a quarter of a million pixels, which is a quarter of a million stack frames the other way.
The region it reaches is a pure function of the sheet and the pixel pressed, so a replay works the whole of it out again from the click exactly as it works a single painted pixel out; asked to fill with the colour already there it spreads nothing, which is what keeps the walk from looking forever for pixels it has just stopped changing.
Held and dragged it fills from every pixel the drag crosses, and the second fill onward costs nothing wherever the first already reached.

**Where the sheet sits and how far in it is zoomed is simulation state**, and `atlas_editor::CanvasView` is `game::Camera`'s counterpart for exactly its reason: which pixel a click means depends entirely on the view, so a view a renderer owned would leave a replay resolving recorded clicks against a different one.
That is also why zoom is an index into a table of whole scales rather than a scale factor, and why `floorDiv` does the mapping from a window pixel to a sheet pixel -- never a float.

## What the toolbar does

The bar along the top holds the four tools, the palette, and the buttons `-`, `+`, `fit`, `grid`, `guides`, `load` and `save`.
The palette is compiled in rather than loaded, exactly as `antwika::i18n`'s catalogues are: this application opens the one file it was asked to edit and no others.
Every swatch is fully opaque, because a transparent pixel is what `ERASE` leaves and a clear swatch would be a second way of saying one thing.

**The sheet is drawn under the whole bar**, since `antwika::gfx` has no clipping and paint order is the only depth there is.
A press the bar covers therefore never reaches the art -- otherwise every button press would leave a dot on it.

Under the buttons is one line saying what would happen and where: the selected tool, the pixel under the pointer, **which slot that pixel is in**, the zoom, the sheet's size, whether there is anything unsaved, and what the last save or load came to.
The slot number is counted left to right then top to bottom from zero, which is exactly how `game::TileAtlas.hpp` addresses the sheet -- so the number here is the number that header names.
A pixel in the strip along an edge too narrow for a whole slot reads `slot -`, because it belongs to no slot the game will ever blit.

## Selecting, moving, and the clipboard

`SELECT` marks a rectangle of the sheet out and carries it somewhere else, and it is the one tool whose left button is a *gesture* -- down, along, and up -- rather than a brush applied to every pixel a drag crosses.

- **Drag from outside the marked rectangle**: draws a new one, both corners included, so a press and a release on one pixel select that pixel.
- **Drag from inside it**: carries it, and the pixels land where the button comes up.
- **Right click**: drops the marked rectangle, whichever tool is in hand -- a rectangle marked with `SELECT` outlives a trip to the palette, so the button that clears it has to as well.
- **Ctrl+C / Ctrl+X**: take a copy of the marked pixels, and cut clears where they came from.
- **Ctrl+V**: puts the clipboard down with its corner **under the pointer**, and marks out where it landed.

Pasting at the pointer rather than back where it came from is what the whole thing is for: carrying a sprite from one slot to another is the move this editor exists to make cheap, and the walk cycles and the sixteen road junctions are mostly variants of one another.
The pixel under the pointer is legitimately simulation state here, since this is the one application in the tree that thins nothing out of its recording -- every movement is recorded, so a replay works out the same pixel; an application attaching `IdleMotionSource` could not do this.

**Nothing about a selection, a drag or the clipboard is persisted**, and no event is added for any of it.
What a recording holds is the press, the movements and the release; which rectangle they came to, which pixels it lifted and where they landed are all worked out again on replay, exactly as a painted pixel is.
That is also why a drag changes no pixel until the button comes up: the outline follows the pointer and the sheet waits, so one gesture is one edit rather than one per movement the window system happened to report.

**A paste writes straight over what was there, transparency included, rather than compositing.**
Putting one slot's art into another means *replacing* it, and a paste that let the old art show through the new one's gaps could not do that at all.

A marked rectangle is only ever set through `clampedTo()`, so it is inside the sheet whenever it holds anything -- which is what lets copy, cut and carry index the image directly.
A drag that leaves the sheet marks the part of it that did not, and one that leaves entirely marks nothing; a carry right off the sheet moves nothing rather than dropping the art off an edge.
A load clears the selection, since a rectangle on the old sheet is not one on the new sheet, whose size need not even be the same -- the clipboard survives, being nobody's sheet in particular.

**There is still no undo, and cut is exactly the case that makes that worth restating.**
Replaying a session up to a point is the undo this design has, and a cut is one recorded chord like any other press.

## The sprite guides, and the one thing a slot grid cannot say

`--tile` divides the sheet into slots, and the slot grid drawn from it says which *sprite* a pixel is in.
It cannot say where inside that sprite the cell it lands on is -- and that is the whole of what [`game-texture-atlas.md`](game-texture-atlas.md) is about.
A sprite is bigger than its diamond: the footprint diamond sits 48 pixels down from the slot's top, with 16 pixels of margin either side and 32 below the pivot for the base block's skirt and its padding.
Painted against the slot boundary alone, a diamond a few pixels out looks fine in the editor and reads as a seam once the game blits it, at one zoom level and not another.

So the `guides` button draws that geometry over every whole slot: the footprint diamond, and a cross on the pivot the blit is anchored by.
`atlas_editor::guidesForTile()` is where a slot size becomes that shape, arithmetically and in one place, exactly as `game::TileAtlas.hpp` is arithmetic rather than a table of rectangles -- so the game's three sheets need no three entries here, only their three `--tile` sizes.

**It refuses rather than approximates, which is why it answers an optional.**
The margins leave a diamond of one width and the two bands leave one of another height, and only an isometric diamond -- twice as wide as it is tall -- makes those two numbers one shape.
A slot size where they disagree, or one with no room for the margins at all, gets no guides drawn rather than a diamond in roughly the right place: nothing here checks the art, so a guide drawn wrong is one an artist paints to, and art painted to it is wrong everywhere the game blits it.

Like the grid, they are a drawing aid and nothing else -- no tool, no click and no saved byte depends on them.
`SpriteGuidesTest` is the second file under `src/apps/atlas_editor/` that names `apps/game`, on `DefaultSheetSizeTest`'s terms and for its reason: it pins every pivot against `game::atlasSpec()`, so a sprite size moved there is a red build here rather than a diamond quietly drawn where no cell is.

## The sheet is uploaded only when it changed

`atlas_editor::Canvas` owns the pixels and counts its changes, and that revision number is half of what `RenderSink` compares against to decide whether to upload the sheet to the renderer again.
Painting a pixel the colour it already holds is not a change, so a stroke that crosses one pixel ten times uploads nothing.

**The other half is the count of loads, and it is not decoration.**
`EditorState::replace()` installs a whole new `Canvas`, which begins at revision zero -- so pressing `load` on a session that has painted nothing moves the revision from zero to zero, and a key made of the revision alone would skip the upload and go on drawing the sheet that is no longer open.
Picking up a file something else changed is most of why anybody presses `load` with nothing unsaved, which is exactly the case that would have lied.

## Saving, loading, and the undo there is not

`save` writes the sheet to `--out` as straight RGBA through `gfx::PngWriter`.
`load` reads `--image` back in, **losing every unsaved change**, and recentres the view on whatever came back.

`atlas_editor::IAtlasStore` is the one seam to a filesystem in this application, and `PngAtlasStore` -- which reads and writes PNGs through `gfx::PngReader` and `gfx::PngWriter` -- is its one implementation.
That is what lets every other class here be exercised with no file on disk at all: a test hands the session a store answering from memory and the session cannot tell the difference.

Neither can end a session.
A save to a full disk, a load of a file somebody has deleted, a save with no `--out` at all: each is reported in the status line and the session carries on with every unsaved change still in hand.
That is the one place this application deliberately catches rather than throwing -- an editor that unwinds out of the tick loop takes an afternoon's work with it.

**There is no undo, and that is a design decision rather than a gap.**
A stack of past sheets would be application state a replay cannot regenerate unless every entry of it is regenerated too.
Every edit already *is* recorded, as the click that made it, so replaying a session up to a point is the undo this design has -- and the opening announcement above is what makes that true against a store that changed since:

```sh
antwika_atlas_editor --image atlas.png --out mine.png --record session.json
antwika_atlas_editor --image atlas.png --out mine.png --replay session.json
```

The mid-session Load button re-reads the store through no event, deliberately: guarding it as the opening is guarded would be a second fingerprint for a button the capped headless runs never press.
A session that pressed Load replays faithfully only while the file it re-read is unchanged -- the one caveat this page's undo claim carries.

**A recording here is large**, and knowingly so.
**This is the one application in the tree that thins nothing out of its recording.**
Most applications attach one or both of `input`'s upstream decorators, but a paint tool can attach neither: `CoalescingPointerSource` would drop every pixel of a stroke but the last, and `IdleMotionSource` would freeze the pixel readout between clicks -- and that readout is simulation state, so it may not be driven off `input::PointerHintChannel` the way `apps/game`'s placement ghost is.
The movement between two clicks *is* the stroke, so a `--record` file grows at the window system's rate rather than the tick rate, which is the price of that.

## The window resizes, and F10 fills the screen

The window is resizable and **F10** toggles fullscreen, and neither changes anything a session can see.

The editor lays out and hit-tests against **1280x720** -- the size the window is *asked* for, which `EditorState::canvas()` carries and `main.cpp` states once.
`RenderSink` builds a `gfx::ViewportRenderer` per frame from the size the window *reports*, so the whole picture -- the sheet, the slot grid, the guides, the selection and the toolbar alike -- is scaled up and centred into whatever the window currently is, with black letterboxes either side.
That is the one place in this application that reads the reported size, and it reads it to place a picture and nothing else; `docs/resizable-windows.md` is the rule, and this is an ordinary application of it.

The pointer is mapped back the other way by `app::WindowPointerMapping`, attached **upstream of the recorder**, so a `--record` file holds canvas positions and nothing about the window it was made on.
A session recorded in a small window replays in a big one, or fullscreen, or the other way about.
The fullscreen toggle itself is `app::FullscreenToggleSource`, above the loop rather than in a sink, because filling the screen is an action on a window and not simulation state -- the *key press* is ordinary recorded input, so a replay fills the screen at the same tick and reaches the same sheet either way.

**One consequence is worth stating plainly, because this is a pixel editor and the unit being edited is one pixel.**
A window whose height is a whole multiple of 720 scales by a whole number, and a click lands on exactly the pixel it would have at 1280x720.
At any other size the scale is a fraction, and a window pixel maps to a sheet pixel and back to *within one* -- so the same gesture can put a dot on the neighbouring pixel.
Below a scale of one it is coarser again, one window pixel then covering more than one of the sheet's.
Zooming the view in is what an artist does about that, and the view's zoom is simulation state, so a replay reproduces it.
None of it costs a replay anything: the recording holds whichever sheet pixel the pointer was actually over, and that is what a replay repaints.
`ViewportReplayTest` is where all of that is asserted end to end, against the pixels rather than against the arithmetic.

## The debug console, and a dump that carries its bitmaps beside it

Grave slides `antwika::console`'s sheet down over the top half of the window, on the library's standard terms: `dump_state` writes the whole session to `dump_state.json` and `load_state` reads it back, live only.
The keys are `console::FixedConsoleControls`' fixed defaults -- Grave, Enter and the Swedish board -- since this application rebinds nothing; Grave conflicts with no editor key, and F10 and Escape stay reserved upstream as before.
`EditorSink` is the one sink here that reads a key or a pixel, so it is the one sink wrapped in a `console::ConsoleGatedSink`: while the console stands over a pixel, a press or a scroll there is the console's, and Ctrl+C, Ctrl+X and Ctrl+V under an open console reach the field rather than the clipboard.

**The sheet and the clipboard are bitmaps, and neither goes into the JSON.**
A 512x768 sheet inline would be megabytes of numbers nobody can read; each is written as a PNG beside the document instead -- `dump_state.sheet.png`, and `dump_state.clipboard.png` when anything is in hand -- and the document binds them with the same `fingerprintOf()` the opening announcement uses.
A load whose PNG does not answer the fingerprint the JSON names is refused whole, exactly as a replay against a changed `--image` is: a dump edited by hand is not repaired.
Everything else the session is goes into the document member for member -- view, tool, colour, swatch, grid and guide toggles, hovered pixel, marked rectangle, a drag mid-gesture, the counters -- except the status line, which is transient and deliberately dropped.

The `atlas.opening_sheet` announcement and `load_state` never meet, and that is by construction rather than by luck: the announcement checks the sheet a *replay* opens against the sheet its recording was drawn on, and `load_state` answers a deterministic refusal in any run that records or replays.
A live run may load whatever it likes -- nothing is checking it against a recording, because there is no recording -- and a replayed run re-executes `dump_state` and rewrites the same files while refusing `load_state`, so the announcement always describes the sheet the run actually opened with.

## How a session ends

Closing the window ends it, and so does Escape; both are input, so both are recorded and both replay -- as F10 does, which ends nothing and fills the screen.
The `null` backend reports neither, which is why there is also `--max-ticks <n>`, defaulting to 90000 -- an hour at the 40 ms frame period.
Reaching it ends the session by *asking it to stop*, through `app::TickLimitSource`, rather than through `EngineLoop`'s own `maxTicks`, which throws.
Running out of the ticks somebody asked for is not a failure, and a `--record` run has to reach its epilogue to save its file at all.
`--max-ticks 0` removes the cap, which is what somebody in front of a real window wants.

## What is left checking the art

Nothing here, deliberately -- the guides above *show* the geometry and check nothing against it.
This editor will happily paint outside the diamond, leave a road stub that meets nothing, or fill a slot the game reads as spare.
[`game-texture-atlas.md`](game-texture-atlas.md) is the contract, `game::requireAtlasSize()` checks the sheet's size at startup, and a person looking at the running game is what catches the rest:

```sh
build-sdl3/bin/antwika_game/antwika_game --replay src/apps/game/replays/demo.jsonl
```

## The config file

`config.json` beside the executable is read once at startup through [`antwika::config`](../libraries/config.md), and holds `framePeriodMs`, how long a frame takes on the wall clock.
The sheet and tile geometry stay in source: the canvas view is simulation state, and what a recorded click paints is worked out from that geometry.
