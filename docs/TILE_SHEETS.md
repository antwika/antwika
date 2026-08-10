# Tile sheet convention

This document describes the spritesheet layout every terrain sheet follows.
The `autotile` library's `SheetLayout.hpp` is the authoritative encoding of this convention, and `scripts/generate_placeholder_tiles.py` renders placeholder sheets that follow it.

## Sheet shape

A sheet is one PNG per terrain, 96 pixels wide and 80 pixels high.
Surfaces are 16x16 display tiles built from 2x2 composable 8x8 quadrants, and the special pieces are single 8x8 half-tiles.
Placeholder sheets are drawn over a transparent background with the single ink color `#d6e0d8`, and the renderer recolors ink and paper per map.

## The corner-mask grid

The left 64x64 region holds the sixteen dual-grid corner masks as a 4x4 grid of 16x16 display tiles.
A mask's tile sits at x = (mask % 4) * 16 and y = (mask / 4) * 16.
Each display tile is drawn centered on a dual-grid corner, so its center is the corner point and its quarters reach into the four surrounding cells.
Bit 1 of the mask is the north-west corner cell, bit 2 the north-east, bit 4 the south-west, and bit 8 the south-east.
Mask 15 is the full interior tile, and it is the only mask with alternate variants.

## The variant tiles at 64,0

The right column holds seven 16x16 interior variants and the water frame in a two-wide, four-tall block.

| Slot | Position | Content |
| --- | --- | --- |
| Surface variant 1 | 64,0 | First alternate of the mask 15 tile. |
| Surface variant 2 | 80,0 | Second alternate. |
| Surface variant 3 | 64,16 | Third alternate. |
| Surface variant 4 | 80,16 | Fourth alternate. |
| Surface variant 5 | 64,32 | Fifth alternate. |
| Surface variant 6 | 80,32 | Sixth alternate. |
| Surface variant 7 | 64,48 | Seventh alternate. |
| Water frame B | 80,48 | The second frame of the water ripple loop. |

`sheetSource(piece, mask, variant)` reaches these slots for Surface pieces with mask 15: variants 1 through 7 pick the alternates, variant 8 picks water frame B, and any other variant falls back to the plain mask tile.
The draw plan scatters every mask 15 tile across the base and the seven variants by a deterministic position hash, with the base weighted at half; water tiles whose scatter lands on the base additionally cycle between the base and frame B on the global clock, while scattered water variants stay static.

## The connection convention

Interior variants are drawn so machinery connects across tile edges no matter which variants land adjacent.
A pipe is two pixels wide, crosses a tile edge only at the edge midpoint — pixels 7 and 8 of the 16-pixel edge — and runs straight inward from any edge it touches for at least four pixels.
A variant that does not use a midpoint leaves those two pixels free of pipe ink, so a neighbour's arriving pipe reads as plunging into the wall mass rather than colliding with off-grid art.
The wall placeholder demonstrates the convention: the mask 15 base is a pipe cross touching all four midpoints (and, at half the tiles, is what guarantees long connected runs), and the variants are a straight horizontal pipe, a straight vertical pipe, a north-east elbow, a south-west elbow, a T opening west-east-south, a valve (a small box on a horizontal pipe), and a tank (an outlined vessel with stubs to all four midpoints), all over a sparse-dot backdrop.
Floor variants carry quieter unconnected details — seams, rivets, a grate, a hatch — and the remaining terrains phase-shift their base pattern per variant.

## The special row at 0,64

| Slot | Position | Content |
| --- | --- | --- |
| Wall band | 0,64 | The vertically tiling cliff-face band, 8x8. |
| Wall rim | 8,64 | The cliff-face rim under a surface edge, 8x8. |
| Bridge deck | 16,64 | The bridge planking drawn over a cell, 8x8. |
| Shade | 24,64 | The dither tile the lighting pass draws, 8x8. |

The renderer tints Shade draws black, so the slot's ink only defines the dither shape.
The rest of the bottom strip, from 32,64 to 96,80 and under the specials, is spare.

## Sidecar

`assets/tiles/tiles.json` records the sheet size, the mask grid, and the named slot positions for tools that do not link the C++ layout.

## Legacy sheets

A PNG with the pre-display-tile 32x48 layout or the short-lived 96x64 layout is refused with a "legacy sheet layout, redraw needed" warning and the placeholder is used instead.

## Regeneration

Run `scripts/generate_placeholder_tiles.py` to rewrite every placeholder sheet and the sidecar into `assets/tiles/`.
The demo and editor apps build the same sheets procedurally in their `PlaceholderSheets.cpp` files, so the script and those files must change together.
