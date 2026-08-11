# Map editor guide

This guide covers `antwika_map_editor`, the authoring tool for Wakewater maps, tilesets, and characters.
It reflects the state documented in [MAP_EDITOR_TASKS.md](MAP_EDITOR_TASKS.md); the design behind it lives in [MAP_SYSTEM_REQUIREMENTS.md](MAP_SYSTEM_REQUIREMENTS.md).

## Starting the editor

Launch from the repository root so the default asset paths resolve:

```sh
./build/bin/antwika_map_editor/antwika_map_editor --map assets/maps/demo.json
```

Without `--map` the editor opens a fresh 20×11 map — a blank, fully pinned floor field at height 0 — and saves to `map.json`.
`--tilesets <dir>` and `--characters <dir>` override the default `assets/tilesets` and `assets/characters` directories.
The window can be freely resized by dragging, F10 toggles fullscreen, and View > UI Scale picks an exact 2x, 3x, or 4x window; scale, fullscreen, and the key bindings persist in a config file between runs.

## The screen

A menu bar (File, Edit, View, Map) runs along the top.
The map viewport fills the left of the canvas and the tool panel the right.
Tab cycles between the three views: Map, Tiles, and Characters, and Shift+Tab cycles them in reverse; Escape always backs out of the innermost thing first (dialog, then menu, then field edit, then quit).
Inside the map bounds the background is a subtle checkerboard; the solid dark area beyond it is outside the map.
The cell under the pointer carries a thin yellow outline so you always see where a click will land.
A hint line in the bottom-left corner describes whatever the cursor points at: the hovered cell with its terrain, height, and annotations, a panel button's purpose, a menu entry, or the sheet slot and pixel in the drawing workspaces.

## Editing the map

Pick a terrain in the panel palette or with keys 1 to 6 (floor, wall, water, cliff, path, stair); key 7 is the free brush described under generation.
The map is a stack of height levels, and you always draw on one of them: the active level, shown in the panel and stepped with E and Q (or the L+ and L- buttons).
Left-click places a slab of the picked terrain at the active level in the hovered cell (dragging paints continuously), and right-click erases the active level's slab; cliff faces between levels render automatically.
A cell can hold slabs on several levels at once, which is how overhangs work: paint a floor at level 0, step up to level 2 or higher, and paint a roof over it — the gap in between stays walkable, because the player is one level tall, so a single level of air is enough.
Hovering a cell cuts away everything above the active level, so working inside a tunnel automatically opens its roof; step the level up to the roof to work on the roof itself.
When the active level is not 0, every column holding a slab there is outlined so the working plane is visible.
B toggles a bridge overlay and L cycles the light level (255, 160, 64) on the active level's slab, rendered as a darkness dither.
U and R are undo and redo; every action, including generation and map growth, reverts as one step.
S saves; the File menu offers New, Open..., Save, Save As..., and Quit, with Open and Save As using a file browser dialog.
New replaces the map with the blank, fully pinned floor field, so Generate does nothing until you mark free cells with brush 7.
The Pick button in the panel (or I) toggles the sprite picker: left-clicks then pick instead of paint, selecting the clicked sprite in the Tiles view so a Tab lands with it ready to edit.
Clicking the same spot again walks up the draw stack — the base sprite first, then the decor and overlapping higher-level sprites above it — and the hint line says what each click will take.
While the picker is on, and for a few seconds after any pick, a small box above the hint line shows the picked sprite's art and its tileset, layer, and sprite number.
Ctrl+left-click picks once without entering the mode, and Escape or a second toggle leaves it.
A pick that resolves to the built-in placeholder (an unbound terrain with no default tileset) selects nothing and reports "placeholder tileset - not editable".

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
Entities are placed on the active level and their markers draw on their own level, so a spawn stays attached to its surface while you work elsewhere.
X removes all entities on the hovered cell.
Clicking a cell that holds an entity selects it instead of painting; the panel then shows editable fields (id, and per kind: target map and entry, item tags, or a character name for spawns).
Edits apply on Enter through the undo history; Escape cancels the field.
Place a spawn early: the validator uses the first spawn as its entry point.

### Stamps

Press [ at one corner and ] at the opposite corner to copy a rectangle of cells.
P pastes the stamp with its top-left at the hovered cell.
Stamps copy whole columns — every level at once — and paste at their original levels regardless of the active level.

### Selections

The dashed-rectangle Select tool beside the picker switches left-clicks from painting to selecting.
With it active, drag a rectangle to marquee cells; releasing keeps the selection outlined on the active level's plane, and a plain click clears it.
Ctrl+C copies the selection, Ctrl+X cuts it, and Ctrl+V pastes the clipboard with its top-left at the hovered cell; pasting works in any tool once the clipboard holds something.
Map selections copy whole columns — every level at once — and paste level-absolute like stamps, clipped at the map bounds.
Cut columns lose every slab and become pinned, so Generate leaves the holes alone.
Pressing inside the selection and dragging moves its content by the drag delta as one undo step, emptying and pinning the vacated cells and clipping at the bounds.
Undo and redo clear the selection, and Escape clears the selection first, then leaves the Select tool, before backing out further.
The Tiles and Characters views carry the same tool for their pixel workspaces; those two share one pixel clipboard, separate from the map's column clipboard.

## Directed generation

Every cell you paint is pinned: the generator must keep it.
The free brush (key 7) marks cells the generator owns instead; free cells show a quiet corner tick and the panel counts them.
Map > Generate (or G) fills all free cells at the active level with the WFC solver, respecting your pins and the terrain adjacency rules; each press explores a new layout, and generated cells stay free so you can re-roll until it feels right.
A pinned cell with no slab at the active level is an authored hole: the generator neither fills it nor lets it constrain its neighbours.
Pin the parts you like by painting over them once.
If the pins contradict each other the map is left untouched and a "generate failed" notice appears.
The pin/free mask saves with the map, so a generation setup survives reload; maps saved before this feature load fully pinned.

## Validation

V toggles the validator overlay: unreachable walkable cells fill red, findings with a location get a red outline, and the panel lists up to six messages.
Map > Validate Now forces an immediate re-check.
The validator reasons per surface, so a tunnel floor and the roof above it are tracked separately: it models one-way cliff drops, stair climbs, bridges, swimmable water, currents, headroom (a roof directly above a floor blocks it), and ability tags granted by pickups; a key in an unreachable spot is a finding.
Falling always lands on the topmost surface below you, so a drop next to a tunnel lands on its roof, never inside it.
The playtest game does not enforce one-way drops, so trust the validator over what the game lets you walk.
Cross-map checks (transition pairs) run via `antwika_mapcheck_cli` and as the `map_assets_validate` ctest over `assets/maps`.

## The palette

Map > Palette... opens the color picker for the map's two colors: ink (the art) and paper (the background).
Pick with the hue slider and the saturation square, or type an exact hex value; the map recolors live behind the dialog.
Apply commits as one undoable step and Cancel restores; the colors save in the map file, so each map keeps its own mood.

## Tilesets

View > Tiles (or Tab) opens the tileset workspace.
A tileset is a named, growable library of 8×8 sprites bound to one terrain; a map can bind a different tileset to each terrain, so several wall styles can coexist in `assets/tilesets/`.
File > New Tileset... creates an empty one (pick a name and its terrain), the panel dropdown switches between open tilesets, and S or File > Save Tileset writes `assets/tilesets/<name>/`.

The left side shows the selected sprite in a zoomed pixel editor with the library grid beside it; the trailing + cell adds a sprite, and the wheel pages when the library outgrows one screen.
Left-click paints the active draw color — ink or paper, picked with the panel swatches or the C key and always shown in the palette's live colors — and right-click clears to transparency.
Every edit updates the map view live.
Sprites are stored as ink, paper, and transparency, and tinted by the map's palette at render time.
The fourth tool tab is Select: marquee a pixel rectangle in the sprite editor of the active frame to move it by dragging or cut, copy, and paste it with Ctrl+X/C/V, where blank clipboard pixels never overwrite what they land on and the clipboard is shared with the Characters view.
The weight stepper under the sprite buttons tunes how often the selected sprite is chosen relative to its peers, from 1 to 16 with a default of 4.
Weights bias every spot where assembly picks among several valid sprites — interchangeable base variants and competing decor alike — without overriding the socket rules.

### Sockets

How sprites may sit next to each other is decided by named edge sockets: two sprites can touch only where their facing edges carry the same socket name.
The colored bands around the pixel editor show the selected sprite's four sockets, and each library cell carries matching edge ticks, so coverage is visible at a glance.
With the Sock tool, add a socket name in the panel, click its row to make it active, then click an edge band to apply it; clicking the band again clears the edge back to open, and right-click always clears.
Two names are reserved: `edge` (black and yellow) marks a side that faces out of the terrain region — borders emerge from it, so give every outward-facing border sprite an `edge` side — and `open` (gray) marks a decor edge that faces emptiness.
Map > Rules... still edits the generation weights and terrain adjacency for the WFC generator; those rules live in rules.json beside the tilesets and are not part of map undo.

### Layers and decor

Layer 0 is the base layer that tiles every cell of a region; +Lay adds decor layers above it.
Decor sprites scatter automatically wherever their rules allow: with the Decr tool the library shows the base sprites, and clicking them toggles which ones the selected decor sprite may sit on.
The layer's density stepper tunes how often decor appears; multi-part decor pieces chain through their own named sockets, while all-`open` decor stands alone.

### Generated preview

Below the frame strip, a small always-on panel shows the selected sprite sitting in the middle of a combination generated from its socket rules, at the map's 1x scale.
A thin outline marks the center cell, `edge` sides on the selected sprite carve the region away to the transparent checker, and every decor layer composites on top so scatter rules are visible too.
A selected decor sprite previews pinned over a base picked from its allowlist; while the allowlist is empty the panel shows the plain neighborhood and the message "no base sprites allowed yet".
The regen button rolls a new arrangement, and the auto checkbox keeps rolling fresh ones every second and a half until unchecked.

### Animation

Each sprite can carry up to four animation frames, selected with the frame buttons or keys 1 to 4 in this view; frame 1 is the sprite as it always draws.
The first stroke on an absent frame creates it as a copy of frame 1, the small previews show all four with the anim box looping them, and Clr clears frame 1 or removes the selected frame and every frame after it.
Frames cycle on a global clock in the editor and the game — the placeholder water shows the effect.

### Binding tilesets to a map

Map > Tilesets... chooses which tileset draws each terrain of the current map; Apply commits as one undoable step and the choice saves in the map file.
Unbound terrains fall back to `default-<terrain>`, and missing tilesets fall back to a built-in placeholder, so a map always renders.
The on-disk format is documented in [TILESETS.md](TILESETS.md).

## Drawing characters

View > Characters manages the character roster.
New creates a character under the name in the field (a placeholder silhouette that already walks); Delete asks for a confirming second click before removing the files.
A character is a 64×64 sheet of 16×16 frames: rows are walk down, up, left, and right; columns are frames 0 to 3, and frame 0 doubles as the idle pose.
The workspace edits pixels exactly like the tile view — left-click paints the active ink or paper color (panel swatches or C), right-click erases to transparency — and characters render in the map palette's colors everywhere they appear.
A small Draw/Select toolbar in the panel offers the same pixel marquee as the tiles view, sharing its clipboard, so art can move between characters and tile sprites with Ctrl+C and Ctrl+V.
A preview beside the workspace plays the animation while you draw.
Characters save as `assets/characters/<name>.png` plus a JSON sidecar; a character named `player` becomes the playable sprite in the game.
The sidecar carries a schema version (a sidecar without one counts as version 1), so future format changes can migrate old characters instead of breaking them.

## Playtesting

F5 (or Map > Playtest) saves the map and launches `antwika_game` on it.
The game loads the tilesets under `assets/tilesets` (or `--tilesets <dir>`) and renders each terrain with the map's bound tileset, falling back to `default-<terrain>` and then to the built-in placeholders, so the playtest shows the same art as the editor, decor and animation included.
In the game the arrow keys move, Escape quits, and F10 toggles fullscreen; movement is blocked by walls and unbridged water, stairs climb one level, and any drop lands on the topmost surface below.
Walking behind tall terrain or under an overhang cuts the occluding block away, so tunnel interiors stay visible.

## The console

The grave key (`) opens the developer console in both apps; Enter runs a command and `help` lists them.
The game offers `map <path>`, `tp <column> <row>`, `pos`, `palette <ink|paper> <hex>`, and `quit`.
The editor offers `open`, `save`, `generate [seed]`, `validate`, `scale <2|3|4>`, `palette`, and `quit`.

## Keyboard reference

The letter and function hotkeys below show their default bindings; Edit > Keys... rebinds them, and the bindings persist in the config file.
Digits, Tab, Escape, the grave console, and the arrows are fixed.

| Key | Action |
| --- | --- |
| 1-6 | Terrain brushes (Map view) |
| 1-4 | Animation frames (Tiles view) |
| 7 | Free brush (generator-owned cells) |
| E / Q | Active level up / down |
| B / L | Bridge / light cycle |
| U / R | Undo / redo (map or tileset, per view) |
| S / O | Save / reload (S saves the tileset in the Tiles view) |
| G | Generate |
| V | Validator overlay |
| T / N / K | Place transition / npc / key pickup |
| X | Delete entities at cell |
| [ / ] / P | Stamp corner / copy / paste |
| C | Toggle the ink/paper draw color |
| I | Sprite picker (Map view) |
| Tab / Shift+Tab | Cycle Map, Tiles, Characters forward / back |
| F5 | Save and playtest |
| F10 | Fullscreen |
| ` | Console |
| Escape | Back out (dialog, menu, field, quit) |

## Tips and known quirks

Save often: the editor has deep undo but no crash recovery.
A region one cell wide needs sprites with `edge` sockets on two opposite sides; the default tilesets only ship interiors, edges, and corners, so build regions two cells thick or author the opposite-edge sprites yourself.
Hovering tall terrain selects the cell behind what you visually point at, because hover mapping ignores the elevation lift; zoom in when working on high ground.
A stair cannot climb into a space with a roof directly above its landing, because the player needs one level of headroom.
Wall tops are never walkable, so paint a roof you want to cross with floor or path terrain; a wall roof only blocks and shelters.
Very fast mouse clicks can be dropped by the input backend, so click deliberately.
The void where a cutaway hides a block, and the plain look of generated regions, are placeholders awaiting real art.
