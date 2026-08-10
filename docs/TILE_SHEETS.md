# Tile sheet convention

This document describes the spritesheet layout every terrain sheet follows.
The `autotile` library's `SheetLayout.hpp` is the authoritative encoding of this convention, and `scripts/generate_placeholder_tiles.py` renders placeholder sheets that follow it.

## Sheet shape

A sheet is one PNG per terrain, 32 pixels wide and 48 pixels high.
Every sprite on it is an 8x8 half-tile, so the sheet is a 4x6 grid of slots.
Placeholder sheets are drawn over a transparent background with the single ink color `#d6e0d8`, and the renderer recolors ink and paper per map.

## The corner-mask grid

Rows y=0 through y=31 hold the sixteen dual-grid corner masks in a 4x4 grid.
A mask's slot sits at x = (mask % 4) * 8 and y = (mask / 4) * 8.
Bit 1 of the mask is the north-west corner, bit 2 the north-east, bit 4 the south-west, and bit 8 the south-east.
Mask 15 is the full interior tile, and it is the only mask with alternate variants.

## The special row at y=32

| Slot | Position | Content |
| --- | --- | --- |
| Wall band | 0,32 | The vertically tiling cliff-face band. |
| Wall rim | 8,32 | The cliff-face rim under a surface edge. |
| Bridge deck | 16,32 | The bridge planking drawn over a cell. |
| Shade | 24,32 | The dither tile the lighting pass draws. |

The renderer tints Shade draws black, so the slot's ink only defines the dither shape.

## The variant row at y=40

| Slot | Position | Content |
| --- | --- | --- |
| Surface variant 1 | 0,40 | An alternate of the mask 15 tile. |
| Surface variant 2 | 8,40 | A second alternate of the mask 15 tile. |
| Water frame B | 16,40 | The second frame of the water ripple loop. |
| Spare | 24,40 | Reserved and left blank. |

`sheetSource(piece, mask, variant)` reaches these slots for Surface pieces with mask 15: variant 1 and 2 pick the alternates, variant 3 picks water frame B, and any other variant falls back to the plain mask tile.
The draw plan scatters variants 1 and 2 by a deterministic position hash, and cycles water tiles between variant 0 and variant 3 on the global clock.

## Sidecar

`assets/tiles/tiles.json` records the sheet size, the mask grid, and the named slot positions for tools that do not link the C++ layout.

## Regeneration

Run `scripts/generate_placeholder_tiles.py` to rewrite every placeholder sheet and the sidecar into `assets/tiles/`.
The demo app builds the same sheets procedurally in `src/apps/tilemap_demo/src/PlaceholderSheets.cpp`, so the script and that file must change together.
