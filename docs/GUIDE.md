# Map editor guide

This guide covers `antwika_map_editor`, the authoring tool for Wakewater maps, tile sheets, and characters.
It reflects the state documented in [MAP_EDITOR_TASKS.md](MAP_EDITOR_TASKS.md); the design behind it lives in [MAP_SYSTEM_REQUIREMENTS.md](MAP_SYSTEM_REQUIREMENTS.md).

## Starting the editor

Launch from the repository root so the default asset paths resolve:

```sh
./build/bin/antwika_map_editor/antwika_map_editor --map assets/maps/demo.json
```

Without `--map` the editor opens a fresh 20×11 map with a pinned wall border and saves to `map.json`.
`--tiles <dir>` and `--characters <dir>` override the default `assets/tiles` and `assets/characters` directories.
The window can be freely resized by dragging, F10 toggles fullscreen, and View > UI Scale picks an exact 2x, 3x, or 4x window; scale and fullscreen persist in a config file between runs.

## The screen

A menu bar (File, Edit, View, Map) runs along the top.
The map viewport fills the left of the canvas and the tool panel the right.
Tab cycles between the three views: Map, Tiles, and Characters; Escape always backs out of the innermost thing first (dialog, then menu, then field edit, then quit).
Inside the map bounds the background is a subtle checkerboard; the solid dark area beyond it is outside the map.
The cell under the pointer carries a thin yellow outline so you always see where a click will land.
A hint line in the bottom-left corner describes whatever the cursor points at: the hovered cell with its terrain, height, and annotations, a panel button's purpose, a menu entry, or the sheet slot and pixel in the drawing workspaces.

## Editing the map

Pick a terrain in the panel palette or with keys 1 to 6 (floor, wall, water, cliff, path, stair); key 7 is the free brush described under generation.
Left-click paints the hovered cell and dragging paints continuously.
E and Q raise and lower the hovered cell's height; cliff faces between heights render automatically.
B toggles a bridge overlay and L cycles the cell's light level (255, 160, 64), rendered as a darkness dither.
U and R are undo and redo; every action, including generation and map growth, reverts as one step.
S saves; the File menu offers New, Open..., Save, Save As..., and Quit, with Open and Save As using a file browser dialog.

### Camera

The mouse wheel zooms the map between 0.5x and 4x, anchored at the cursor.
Middle-button drag pans, and horizontal scroll pans sideways.
The panel, menus, and text never zoom with the map.

### Growing the map

Maps extend after the fact in every direction.
Hover just past the map's edge and a ghost cell appears; painting it grows the map to include it, shifting nothing visually.
Entity coordinates are adjusted automatically when the map grows north or west, and undo reverts the growth together with the stroke.

### Entities

Choose a kind in the panel dropdown (transition, boat, spawn, pickup, npc, trigger) and press Place, or use T, N, and K as fast paths for transition, npc, and key pickup.
X removes all entities on the hovered cell.
Clicking a cell that holds an entity selects it instead of painting; the panel then shows editable fields (id, and per kind: target map and entry, item tags, or a character name for spawns).
Edits apply on Enter through the undo history; Escape cancels the field.
Place a spawn early: the validator uses the first spawn as its entry point.

### Stamps

Press [ at one corner and ] at the opposite corner to copy a rectangle of cells.
P pastes the stamp with its top-left at the hovered cell.

## Directed generation

Every cell you paint is pinned: the generator must keep it.
The free brush (key 7) marks cells the generator owns instead; free cells show a quiet corner tick and the panel counts them.
Map > Generate (or G) fills all free cells with the WFC solver, respecting your pins and the terrain adjacency rules; each press explores a new layout, and generated cells stay free so you can re-roll until it feels right.
Pin the parts you like by painting over them once.
If the pins contradict each other the map is left untouched and a "generate failed" notice appears.
The pin/free mask saves with the map, so a generation setup survives reload; maps saved before this feature load fully pinned.

## Validation

V toggles the validator overlay: unreachable walkable cells fill red, findings with a location get a red outline, and the panel lists up to six messages.
Map > Validate Now forces an immediate re-check.
The validator models one-way cliff drops, stair climbs, bridges, swimmable water, currents, and ability tags granted by pickups; a key in an unreachable spot is a finding.
The playtest demo does not enforce one-way drops, so trust the validator over what the demo lets you walk.
Cross-map checks (transition pairs) run via `antwika_mapcheck_cli` and as the `map_assets_validate` ctest over `assets/maps`.

## The palette

Map > Palette... opens the color picker for the map's two colors: ink (the art) and paper (the background).
Pick with the hue slider and the saturation square, or type an exact hex value; the map recolors live behind the dialog.
Apply commits as one undoable step and Cancel restores; the colors save in the map file, so each map keeps its own mood.

## Drawing tiles

View > Tiles (or Tab) opens the pixel workspace for the selected terrain's 96×80 sheet.
The grid shows every pixel; strong guide lines mark the 16×16 display tiles — the 4×4 corner-mask grid on the left, the seven variant tiles plus the water frame on the right, and the band, rim, bridge, and shade half-tiles on the bottom strip — while fainter lines keep the 8×8 quadrant structure visible inside every mask tile.
Left-click inks a pixel and right-click clears it; art is strictly one-bit.
Each 16×16 mask tile draws centered on a dual-grid corner, and its mask encodes which of the four surrounding cells hold the terrain: draw the full tile (mask 15) first, then the edges.
Interior machinery connects across tiles when pipes cross edges only at the two-pixel edge midpoints, per the connection convention in TILE_SHEETS.md — the wall placeholders ship as connectable pipe pieces that demonstrate it.
Every edit updates the map view live; File > Save Sheet writes `assets/tiles/<terrain>.png`, which the editor and demo load at startup.
Sheets are drawn in white and tinted by the map's ink color at render time.
The layout convention is documented in [TILE_SHEETS.md](TILE_SHEETS.md).

## Drawing characters

View > Characters manages the character roster.
New creates a character under the name in the field (a placeholder silhouette that already walks); Delete asks for a confirming second click before removing the files.
A character is a 64×64 sheet of 16×16 frames: rows are walk down, up, left, and right; columns are frames 0 to 3, and frame 0 doubles as the idle pose.
The workspace edits pixels exactly like the tile view — left-click inks, right-click erases to transparency — and characters render in the map's ink color everywhere they appear.
A preview beside the workspace plays the animation while you draw.
Characters save as `assets/characters/<name>.png` plus a JSON sidecar; a character named `player` becomes the playable sprite in the demo.

## Playtesting

F5 (or Map > Playtest) saves the map and launches `antwika_tilemap_demo` on it.
In the demo the arrow keys move, Escape quits, and F10 toggles fullscreen; movement is blocked by walls and water and follows cell heights, and walking behind tall terrain cuts the occluding block away.

## The console

The grave key (`) opens the developer console in both apps; Enter runs a command and `help` lists them.
The demo offers `map <path>`, `tp <column> <row>`, `pos`, `palette <ink|paper> <hex>`, and `quit`.
The editor offers `open`, `save`, `generate [seed]`, `validate`, `scale <2|3|4>`, `palette`, and `quit`.

## Keyboard reference

| Key | Action |
| --- | --- |
| 1-6 | Terrain brushes |
| 7 | Free brush (generator-owned cells) |
| E / Q | Raise / lower height |
| B / L | Bridge / light cycle |
| U / R | Undo / redo |
| S / O | Save / reload |
| G | Generate |
| V | Validator overlay |
| T / N / K | Place transition / npc / key pickup |
| X | Delete entities at cell |
| [ / ] / P | Stamp corner / copy / paste |
| Tab | Cycle Map, Tiles, Characters |
| F5 | Save and playtest |
| F10 | Fullscreen |
| ` | Console |
| Escape | Back out (dialog, menu, field, quit) |

## Tips and known quirks

Save often: the editor has deep undo but no crash recovery.
Walls one cell thick render as separated blobs under the dual-grid scheme, so build walls two cells thick where the look matters.
Hovering tall terrain selects the cell behind what you visually point at, because hover mapping ignores the elevation lift; zoom in when working on high ground.
Very fast mouse clicks can be dropped by the input backend, so click deliberately.
The void where a cutaway hides a block, and the plain look of generated regions, are placeholders awaiting real art.
