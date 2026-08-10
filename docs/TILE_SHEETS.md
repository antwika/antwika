# Tile sheet convention

This document describes the spritesheet layout every terrain sheet follows.
The `autotile` library's `SheetLayout.hpp` is the authoritative encoding of this convention, and `scripts/generate_placeholder_tiles.py` renders placeholder sheets that follow it.

## Sheet shape

A sheet is one PNG per terrain, 96 pixels wide and 64 pixels high.
Surfaces are 16x16 display tiles built from 2x2 composable 8x8 quadrants, and the special pieces are single 8x8 half-tiles.
Placeholder sheets are drawn over a transparent background with the single ink color `#d6e0d8`, and the renderer recolors ink and paper per map.

## The corner-mask grid

The left 64x64 region holds the sixteen dual-grid corner masks as a 4x4 grid of 16x16 display tiles.
A mask's tile sits at x = (mask % 4) * 16 and y = (mask / 4) * 16.
Each display tile is drawn centered on a dual-grid corner, so its center is the corner point and its quarters reach into the four surrounding cells.
Bit 1 of the mask is the north-west corner cell, bit 2 the north-east, bit 4 the south-west, and bit 8 the south-east.
Mask 15 is the full interior tile, and it is the only mask with alternate variants.

## The variant tiles at 64,0

| Slot | Position | Content |
| --- | --- | --- |
| Surface variant 1 | 64,0 | An alternate of the mask 15 tile, 16x16. |
| Surface variant 2 | 80,0 | A second alternate of the mask 15 tile, 16x16. |
| Water frame B | 64,16 | The second frame of the water ripple loop, 16x16. |
| Spare | 80,16 | Reserved and left blank. |

`sheetSource(piece, mask, variant)` reaches these slots for Surface pieces with mask 15: variant 1 and 2 pick the alternates, variant 3 picks water frame B, and any other variant falls back to the plain mask tile.
The draw plan scatters variants 1 and 2 by a deterministic position hash, and cycles water tiles between variant 0 and variant 3 on the global clock.

## The special row at 64,32

| Slot | Position | Content |
| --- | --- | --- |
| Wall band | 64,32 | The vertically tiling cliff-face band, 8x8. |
| Wall rim | 72,32 | The cliff-face rim under a surface edge, 8x8. |
| Bridge deck | 80,32 | The bridge planking drawn over a cell, 8x8. |
| Shade | 88,32 | The dither tile the lighting pass draws, 8x8. |

The renderer tints Shade draws black, so the slot's ink only defines the dither shape.
The region below the special row, from 64,40 to 96,64, is spare.

## Sidecar

`assets/tiles/tiles.json` records the sheet size, the mask grid, and the named slot positions for tools that do not link the C++ layout.

## Legacy sheets

A PNG with the pre-display-tile 32x48 layout is refused with a "legacy sheet layout, redraw needed" warning and the placeholder is used instead.

## Regeneration

Run `scripts/generate_placeholder_tiles.py` to rewrite every placeholder sheet and the sidecar into `assets/tiles/`.
The demo and editor apps build the same sheets procedurally in their `PlaceholderSheets.cpp` files, so the script and those files must change together.
