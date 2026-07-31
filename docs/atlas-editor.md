# The atlas editor: painting the sheet the game blits

`antwika_atlas_editor` is a pixel editor for the texture atlas `apps/game` draws itself from.
It exists because that sheet is hand-drawn art and is the source of truth for how the game looks -- see [`game-texture-atlas.md`](game-texture-atlas.md), which is the contract the art has to meet and which this editor does nothing to enforce.
What it gives an artist is the one thing an outside tool cannot: the slot boundaries the game addresses by, drawn over the sheet, so the tile being painted is the tile `game::TileAtlas.hpp` will blit.

It is an ordinary application of this codebase rather than a special case.
One tick is one frame, a click is the only input, `--record` and `--replay` work as they do everywhere else, and every pixel it puts down is regenerated from the recorded clicks rather than persisted.

## Opening a sheet

```sh
build/bin/antwika_atlas_editor/antwika_atlas_editor \
    --image src/apps/game/assets/atlas.png \
    --out my-atlas.png
```

`--image` is the PNG to open and `--out` is the PNG a save writes.
**They are deliberately separate, and neither defaults to the other.**
The sheet somebody is most likely to open first is the game's own, and one stray click on `save` should not be able to replace the art with a half-finished experiment.
Naming `--out` is how an artist says which file is theirs; without it the `save` button reports that there is nowhere to write to and changes nothing.

Given no `--image`, the session starts on a blank, fully transparent sheet of `--sheet <w>x<h>`, which defaults to the game's own 1024x256.

`--tile <w>x<h>` is what the grid overlay divides the sheet into, defaulting to the game's 128x64.
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

The three tools are `PAINT`, which puts the selected colour down, `ERASE`, which makes a pixel fully transparent, and `PICK`, which takes the colour under the pointer as the one to paint with.
A picked colour that no swatch offers leaves no swatch shown as chosen, rather than the nearest one lighting up and lying about it.

## What the toolbar does

The bar along the top holds the three tools, the palette, and the buttons `-`, `+`, `fit`, `grid`, `load` and `save`.
The palette is compiled in rather than loaded, exactly as `antwika::i18n`'s catalogues are: this application opens the one file it was asked to edit and no others.
Every swatch is fully opaque, because a transparent pixel is what `ERASE` leaves and a clear swatch would be a second way of saying one thing.

**The sheet is drawn under the whole bar**, since `antwika::gfx` has no clipping and paint order is the only depth there is.
A press the bar covers therefore never reaches the art -- otherwise every button press would leave a dot on it.

Under the buttons is one line saying what would happen and where: the selected tool, the pixel under the pointer, **which slot that pixel is in**, the zoom, the sheet's size, whether there is anything unsaved, and what the last save or load came to.
The slot number is counted left to right then top to bottom from zero, which is exactly how `game::TileAtlas.hpp` addresses the sheet -- so the number here is the number that header names.
A pixel in the strip along an edge too narrow for a whole slot reads `slot -`, because it belongs to no slot the game will ever blit.

## Saving, loading, and the undo there is not

`save` writes the sheet to `--out` as straight RGBA through `gfx::PngWriter`.
`load` reads `--image` back in, **losing every unsaved change**, and recentres the view on whatever came back.

Neither can end a session.
A save to a full disk, a load of a file somebody has deleted, a save with no `--out` at all: each is reported in the status line and the session carries on with every unsaved change still in hand.
That is the one place this application deliberately catches rather than throwing -- an editor that unwinds out of the tick loop takes an afternoon's work with it.

**There is no undo, and that is a design decision rather than a gap.**
A stack of past sheets would be application state a replay cannot regenerate unless every entry of it is regenerated too.
Every edit already *is* recorded, as the click that made it, so replaying a session up to a point is the undo this design has:

```sh
antwika_atlas_editor --image atlas.png --out mine.png --record session.json
antwika_atlas_editor --image atlas.png --out mine.png --replay session.json
```

**A recording here is large**, and knowingly so.
Most applications in this tree thin pointer movement out of the stream before it is recorded, but a paint tool cannot: the movement between two clicks *is* the stroke, and the pixel readout follows it.
So a `--record` file grows at the window system's rate rather than the tick rate.

## How a session ends

Closing the window ends it, and so does Escape; both are input, so both are recorded and both replay.
The `null` backend reports neither, which is why there is also `--max-ticks <n>`, defaulting to 90000 -- an hour at the 40 ms frame period.
Reaching it asks the session to stop rather than failing, so a `--record` run still reaches its epilogue and writes its file.
`--max-ticks 0` removes the cap, which is what somebody in front of a real window wants.

## What is left checking the art

Nothing here, deliberately.
This editor will happily paint outside the diamond, leave a road stub that meets nothing, or fill a slot the game reads as spare.
`docs/game-texture-atlas.md` is the contract, `game::requireAtlasSize()` checks the sheet's size at startup, and a person looking at the running game is what catches the rest:

```sh
build-sdl3/bin/antwika_game/antwika_game --replay src/apps/game/replays/demo.json
```
