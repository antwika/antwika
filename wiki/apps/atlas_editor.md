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

- **Left button**: whatever the selected tool is, on the pixel under the pointer, and on every pixel a drag crosses.
- **Right button**: erases, whichever tool is selected, so taking a mistake back never costs a trip to the toolbar.
- **Middle button, dragged**: pans the sheet across the window.
- **Wheel**: zooms in and out, keeping the pixel under the pointer under the pointer.

**"Every pixel a drag crosses" is the whole segment rather than one dot per event**, and that is what the held-button arms actually walk.
A window system reports a fast stroke as a handful of long jumps, so painting only where each event landed left a dotted line with the gaps in between bare -- which is exactly what a fast hand produces and a slow one never does.
The walk is integer Bresenham from where the pointer was to where it is, for the no-floating-point reason below: which pixel a press means is simulation state, and a float's last bit is not the same on every toolchain.
Repainting the pixel the last event already put down costs nothing, since a write of the colour already there is no change at all.

The three tools are `PAINT`, which puts the selected colour down, `ERASE`, which makes a pixel fully transparent, and `PICK`, which takes the colour under the pointer as the one to paint with.
A picked colour that no swatch offers leaves no swatch shown as chosen, rather than the nearest one lighting up and lying about it.

**Where the sheet sits and how far in it is zoomed is simulation state**, and `atlas_editor::CanvasView` is `game::Camera`'s counterpart for exactly its reason: which pixel a click means depends entirely on the view, so a view a renderer owned would leave a replay resolving recorded clicks against a different one.
That is also why zoom is an index into a table of whole scales rather than a scale factor, and why `floorDiv` does the mapping from a window pixel to a sheet pixel -- never a float.

## What the toolbar does

The bar along the top holds the three tools, the palette, and the buttons `-`, `+`, `fit`, `grid`, `load` and `save`.
The palette is compiled in rather than loaded, exactly as `antwika::i18n`'s catalogues are: this application opens the one file it was asked to edit and no others.
Every swatch is fully opaque, because a transparent pixel is what `ERASE` leaves and a clear swatch would be a second way of saying one thing.

**The sheet is drawn under the whole bar**, since `antwika::gfx` has no clipping and paint order is the only depth there is.
A press the bar covers therefore never reaches the art -- otherwise every button press would leave a dot on it.

Under the buttons is one line saying what would happen and where: the selected tool, the pixel under the pointer, **which slot that pixel is in**, the zoom, the sheet's size, whether there is anything unsaved, and what the last save or load came to.
The slot number is counted left to right then top to bottom from zero, which is exactly how `game::TileAtlas.hpp` addresses the sheet -- so the number here is the number that header names.
A pixel in the strip along an edge too narrow for a whole slot reads `slot -`, because it belongs to no slot the game will ever blit.

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

## How a session ends

Closing the window ends it, and so does Escape; both are input, so both are recorded and both replay.
The `null` backend reports neither, which is why there is also `--max-ticks <n>`, defaulting to 90000 -- an hour at the 40 ms frame period.
Reaching it ends the session by *asking it to stop*, through `app::TickLimitSource`, rather than through `EngineLoop`'s own `maxTicks`, which throws.
Running out of the ticks somebody asked for is not a failure, and a `--record` run has to reach its epilogue to save its file at all.
`--max-ticks 0` removes the cap, which is what somebody in front of a real window wants.

## What is left checking the art

Nothing here, deliberately.
This editor will happily paint outside the diamond, leave a road stub that meets nothing, or fill a slot the game reads as spare.
[`game-texture-atlas.md`](game-texture-atlas.md) is the contract, `game::requireAtlasSize()` checks the sheet's size at startup, and a person looking at the running game is what catches the rest:

```sh
build-sdl3/bin/antwika_game/antwika_game --replay src/apps/game/replays/demo.jsonl
```
