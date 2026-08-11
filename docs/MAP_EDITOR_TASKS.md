# Map editor task backlog

This file records the map_editor work queue agreed on 2026-08-10, so the specifications survive even if the session executing them dies mid-stream.
The tasks run strictly in order because each one touches the same app.
A task is not done until it builds clean under -Werror, passes the three style checker scripts, keeps every existing test green, and has been exercised under Xvfb with xdotool-driven synthetic input.
Unit tests for the editor itself remain deferred by explicit decision; see docs/MAP_SYSTEM_PLAN.md for the surrounding plan.
The tasks below name the playtest app `tilemap_demo`, which is what it was called while they shipped; on 2026-08-11 it became `src/apps/game`, the target `antwika_game`, and the namespace `antwika::game`.

## Status

| # | Task | State |
| --- | --- | --- |
| 1 | Menu bar | shipped |
| 1b | Menu click-toggle fix | shipped |
| 2 | Map camera: zoom and pan | shipped |
| 3 | WFC generation directed by pins | shipped |
| 4 | Compact ui theme | shipped |
| 5 | File explorer dialog | shipped |
| 6 | Smaller text face | shipped |
| 7 | Tile sheet workspace | shipped |
| 8 | Character management | shipped |
| 9 | User-selectable UI scale | shipped |
| 10 | Canvas presentation and map extension | shipped |
| 11 | Free-mask persistence, schema v2 | shipped |
| 12 | Runtime keyboard under the raylib backend | shipped |
| 13 | Map palette picker | shipped |
| 14 | Developer console in tilemap_demo and map_editor | shipped |
| 15 | Fullscreen support | shipped |
| 16 | Drag-resizable windows | shipped |
| 17 | Generate-every-second-press bug | fixed |
| 18 | Yellow hover outline | shipped |
| 19 | Pointer hint line | shipped |
| 20 | Disconnected-tiles rendering bug | fixed |
| 21 | Eight interior variants and connected pipes | shipped |
| 22 | Character eraser and ink consistency | fixed |
| 23 | Artist-editable adjacency rules and weights | shipped |
| 24 | Per-variant edge connectors | shipped |
| 25 | Two-color drawing with palette baking | shipped |
| 26 | Ctrl+click edge toggles and outline markers | fixed |
| 27 | Quadrant libraries with 8px edge matching | shipped |
| 28 | Socket-based tilesets with layers, decor, and animation | shipped |
| 29 | Blank New, four frames, reverse view cycling, rebindable hotkeys | shipped |
| 30 | Generated-combination preview panel in the tileset workspace | shipped |
| 31 | Per-level slab columns: overhangs, tunnels, schema v4 | shipped |
| 32 | Artist-controlled per-sprite frequency weights | shipped |
| 33 | Sprite picker with layer-cycling picks in the map view | shipped |
| 34 | Playtest map flag, brush-independent erase, erase hint | fixed |
| 35 | Demo renders the map's bound tilesets with decor and animation | shipped |
| 36 | One-level character clearance | shipped |
| 37 | Icon toolbars with hint-line tool names | shipped |
| 38 | Select tool: marquee, move, cut, copy, paste in all views | shipped |

## 1: menu bar (shipped)

A ui-built bar at the top of the 480×270 canvas: File (New, Save, Load, Quit), Edit (Undo, Redo, Delete Entity), View (Validator toggle), Map (Playtest, Validate Now), with keyboard shortcuts shown in entry labels.
Entries route through the existing undoable command functions only.
Implemented with `ui::Context::dropdown` overlay nodes; open state is app-owned in the store.
The map viewport shifted down by the bar height and bar clicks never reach the map.

## 1b: menu click-toggle fix (shipped)

Bug found by synthetic clicking: the menu opens on mouse press and the release closes it again, so only holding the button keeps it open.
Expected: a completed click leaves the menu open; it closes only on entry choice, a second title click, an outside press, or Escape.
Verification is the xdotool click-wait-capture sequence proving the entries stay visible after release.
Outcome: instrumentation proved the editor's logic was already edge-correct, and the real defect sits in the raylib input backend, whose once-per-frame state diff drops a press-and-release pair that lands inside one poll, so a zero-delay synthetic click delivers no events at all.
The backend is outside the permitted footprint, so verification uses clicks with a human-scale hold (about 60 ms), for which open, stay-open, second-click close, outside close, and Escape all pass.

## 2: map camera — zoom and pan

Wheel over the map viewport zooms through discrete steps (0.5x to 4x) anchored at the cursor; middle-button drag pans (right-drag fallback if the backend never emits middle); horizontal scroll pans horizontally.
Pan clamps so part of the map always stays visible.
The draw plan stays at 1x coordinates; only rendering and hit-testing apply the camera transform, and `cellUnder` inverts it.
Panel, menu bar, and HUD never scale with the map camera.
Verification includes painting the correct cell while zoomed, proven by saving and inspecting the JSON.

## 3: WFC generation directed by pins

Painted cells are pinned constraints; a seventh "free" brush (key 7) unpins cells, marked with a quiet visual tick.
Map > Generate (key G) runs the antwika::wfc solver over free cells only: pinned cells enter as single-value domains.
Generation domain is Floor, Wall, Water, Path, Cliff with weights 8/3/2/2/1; Stair is artist-only but pinned stairs constrain neighbours.
Adjacency (4-way, symmetric): floor touches anything; wall touches wall/floor/path/stair; water touches water/floor; path touches path/floor/wall/stair; cliff touches cliff/floor.
Deterministic seeding from a store-held counter incremented per run; success applies as one undoable command; contradiction leaves the map untouched and reports.
File > New pins only the border; loading a map pins everything (superseded by task 11 for v2 files, whose saved free mask loads verbatim; v1 files still pin everything); generated cells stay free for re-rolls.
Verification: pinned water survives a Generate verbatim in the saved JSON, and undo reverts the whole Generate.
Amendment: the solver is fully deterministic with no rng of its own, so variety comes from seed pins — roughly every sixth free cell is pre-collapsed to a weighted random value drawn from an xorshift32 stream, restricted to values compatible with already-fixed neighbours so the seeded solve cannot contradict; an unseeded solve is the fallback.

## 4: compact ui theme

A denser `ui::Theme` instance local to the editor (roughly padding 4→2, gap 4→2, buttonPadding 6→3, proportional scrollbar/slider shrink); src/libs/ui is not touched because eleven apps share it.
Layout constants derived from theme metrics (menu bar height) are recomputed, not left stale.
The reclaimed panel space grows the findings list.
Shipped values: padding 2, gap 1, buttonPadding 2, scrollbarWidth 4, sliderHeight 8, giving a menu bar height of 12 canvas pixels.

## 5: file explorer dialog

Part A extracts atlas_editor's pure `FileList` helpers (FileEntry, kParentEntry, entriesIn, pathIn, entryText) into src/libs/io as `antwika::io`, moving its existing tests along (they must pass) and updating atlas_editor call sites.
Part B: the File menu becomes New, Open..., Save, Save As..., Quit; Open/Save As raise a modal ui-built explorer: path label, entry list (parent, directories, then *.json files), click-to-navigate, editable filename field, Confirm/Cancel, scrolling or paging for long listings.
While open, everything underneath is suppressed and Escape closes the dialog before meaning anything else.
Verification includes a Save As landing a file at the navigated path on disk.
Amendments at shipping: atlas_editor keeps compiling through a small compat header re-exporting the io names into its namespace, because the call sites are many; long listings page with Prev/Next buttons rather than a scroll region, since the ui library scrolls text areas but not widget lists; the Escape-closes-dialog path is implemented but cannot be exercised under the raylib backend, which emits no keyboard events at all.

## 6: smaller text face

The built-in font is a TTF rasterized at 8px line height by GlyphCells, so a smaller face is the same font at 6px (7px fallback if 6 is illegible in captures).
Additive only: default face and all other apps stay byte-identical; footprint may include gfx, ui, and both backends if drawText needs a face parameter, choosing the least invasive mechanism after reading the whole text path.
`ui::Theme` gains the face field and ui layout measures with the chosen face's metrics, so bar and button heights shrink accordingly.
Amendment at shipping (2026-08-10): the face rides in the existing uint32 text scale (face id in the high bits, multiplier in the low bits), so no drawText signature changed and every plain scale keeps its meaning.
The small face rasterizes the built-in TTF at a 6-pixel line height with a 5-pixel advance, which captures show fully legible, so the 7-pixel fallback was not needed.
The editor's menu bar height drops from 12 to 10 canvas pixels, superseding the value recorded in task 4.

## 7: tile sheet workspace

The editor loads assets/tiles/<terrain>.png (via a --tiles flag, default assets/tiles) as the render source when present and correctly sized, else falls back to the procedural placeholder per terrain.
View > Tiles (Tab) swaps the viewport for a magnified pixel workspace over the selected terrain's 32×48 sheet: slot grid with hover labels per docs/TILE_SHEETS.md, left-click inks, right-click clears, strictly 1-bit, undoable, with the texture re-uploaded on edit so the map view updates live.
File > Save in this view writes the sheet through gfx::PngWriter.
Verification includes the restart round-trip: paint, save, relaunch, and the map renders the new art.
Amendment (2026-08-10): the workspace presentation mirrors atlas_editor's Canvas/TileGrid/SpriteGuides ideas — zoom-gated faint pixel grid, stronger guide lines on the 8×8 slot and row-band boundaries, atlas_editor-style transparency backdrop, and its cheap affordances (hover pixel highlight, ink indicator) where they fit the compact theme — reimplemented locally, not linked, unless a genuinely pure helper justifies a FileList-style extraction.
Amendments at shipping (2026-08-10): the workspace magnification is fixed at 5x (above the 4x grid gate) rather than user-zoomable, the edited sheet follows the terrain brush palette, and each sheet carries its own undo/redo stacks that Edit > Undo/Redo drive while the view is active.
File > Save's label adapts to "Save Sheet" and writes only the selected terrain's sheet; Open and Save As keep operating on maps in every view.
The Tab fast path waits for task 12's keyboard, so View > Tiles/Map is the working toggle, and the raylib backend delivers right-button clears without trouble.

## 8: character management

Convention: one character = assets/characters/<name>.png, a 64×64 sheet of 4×4 16×16 frames; rows walk_down/walk_up/walk_left/walk_right, columns frames 0-3, frame 0 doubling as idle; a JSON sidecar describes the table.
View > Characters (Tab cycles Map/Tiles/Characters): panel lists characters with New (procedural silhouette placeholder, rejecting empty/duplicate names), Delete behind a confirm step that removes PNG and sidecar, and a name field; the viewport reuses the pixel workspace; a panel preview animates the hovered row at ~8 fps.
Spawn entities offer their enemy field as a dropdown of character names if panel space allows.
tilemap_demo renders assets/characters/player.png as the animated player (direction row, cycling while moving, idle at rest) with the orange rect as fallback.
Verification includes running the demo with a painted player.png and capturing the sprite in place of the rect.
Amendment (2026-08-10): the character pixel workspace follows the same atlas_editor-derived presentation as the tile workspace — zoom-gated pixel grid, stronger 16×16 frame guide lines, its transparency backdrop, and transferable affordances — reimplemented locally per the task 7 amendment.
Amendments at shipping (2026-08-10): the character workspace runs at a fixed 4x zoom, and the animated row preview sits in the workspace's left margin rather than the panel, because the ui phase paints the panel above everything the render phase draws; the previewed row follows the workspace hover (walk_down when nothing is hovered) at a tick/8 frame clock.
A --characters flag (default assets/characters) picks the directory, New writes the placeholder PNG and sidecar immediately so Delete always has files to remove, and the name field starts prefilled with "player" since the raylib backend still delivers no typing before task 12 — empty-name rejection therefore ships tested by code path while duplicate rejection was exercised live.
The demo hardcodes assets/characters/player.png relative to its working directory, keeps the orange rect fallback, and the idle sprite render was captured in place of the rect; task 12 later verified arrow-driven movement with the walk frames cycling.
The spawn enemy dropdown shipped with a "(none)" first entry, and the chosen name was verified in the saved JSON.

## 9: user-selectable UI scale

View menu offers UI Scale 2x (960×540), 3x (1440×810, default), 4x (1920×1080); the 480×270 canvas is unchanged and everything scales as crisp integer multiples, with the active choice marked.
Live window resize preferred (an additive IWindow method backed by raylib's SetWindowSize is allowed under the same rules as task 6); the ViewportRenderer transform must be rebuilt on change so pointer hit-testing stays correct, which verification proves by painting after a live switch.
If live resize is impractical, the choice persists and applies at startup with a "restart to apply" note.
The chosen scale persists in a map_editor config file following atlas_editor's ConfigFile precedent.
Amendments at shipping (2026-08-10): live resize worked, through an additive `IWindow::setSize` whose base implementation is a documented no-op, a raylib override calling SetWindowSize, and an additive `ViewportRenderer::resize` rebuilding the transform.
The three scales sit in the View menu with a "*" marking the active one, the config file is `config.json` beside the binary via the ANTWIKA_CONFIG_FILE macro, and painting after a live 3x-to-2x switch landed on the intended cell in the saved JSON.
Under the window-manager-less Xvfb a relaunched window centers itself, which is environmental, not app behavior.

## 10: canvas presentation and map extension

Part A (editor only): a subtle two-shade checkerboard behind the tiles inside map bounds, locked to cell space and transforming with the camera, with one solid darker void color outside the bounds; Shade and validator overlays draw above it unchanged.
Part B: an additive `expandedMap(map, west, north, east, south)` free function in src/libs/tilemap returning a larger map with cells copied at the offset, entities (including trigger extents' origins) shifted by (west, north), header preserved, and zero growth returning an equal map — with unit tests in tilemap's style covering corner copy, entity shift, zero-growth equality, and header preservation, because the deferral covers editor code, not that tested library.
Editor: painting within a few cells beyond an edge shows a ghost cell and auto-extends the map to include it, keeping pins, the free grid, the ECS mirror, and the camera pan consistent (no visual jump on west/north growth), and undo reverts extension plus paint together; explicit Map > Extend entries are optional if auto-extend suffices.
Verification: zoomed-out capture showing checker inside and void outside, painting past the west edge grows columns and shifts a placed entity's column in the saved JSON, and undo restores the old dimensions.
Amendments at shipping (2026-08-10): the undo stacks now snapshot the pin grid beside the map, so one undo reverts extension, pins, and paint together instead of falling back to reconcilePins's all-pinned repair.
Growth from painting defaults the new cells to free, anticipating task 11, and explicit Map > Extend entries were not needed because auto-extend suffices.
All four verification points passed live: ghost cell at a west off-edge hover, 20-to-21 column growth with the entity shifting from column 2 to 3 in the JSON, an undo restoring both, and the checkerboard-inside/void-outside capture at 0.5x zoom.

## 11: free-mask persistence, schema v2

The generation pin/free mask travels with the map file; the Cell model does not change because the mask is serialization-level.
MapJson gains an always-emitted "free" section, one string per row of '.' (pinned) and 'o' (free) in the terrain row style, and kSchemaVersion bumps to 2.
The loader accepts v1 and v2: a v1 document (no "free" member) loads all-pinned, v2 requires the section with matching dimensions, and toJson always writes v2.
The mask rides beside the map in a small MapDocument-style pairing (or parallel functions — implementer's call, reported) with the existing TileMap-only signatures kept working on an all-pinned default.
The tested-lib rule applies: tilemap unit tests must cover the v2 round-trip, v1 migration to all-pinned, dimension-mismatch rejection, and unknown-schema rejection.
mapcheck_cli and tilemap_demo keep using the TileMap-only path and must load v2 untouched; assets/maps/demo.json is re-exported as v2 and map_assets_validate stays green.
In the editor the pinned grid loads from and saves with the file, File > New keeps border-pinned/interior-free, v1 maps load all-pinned, and task 10's expandedMap growth defaults new cells to free with the mask growing in sync.
When this ships, the task 3 rule "loading a map pins everything" is superseded for v2 files, and a note there must say so.
Verification: paint pins and free regions, save, inspect the JSON's free rows, reload and capture the free ticks in the same places, load a v1 map and confirm all-pinned, tilemap tests green, and the standard bars.
Amendments at shipping (2026-08-10): the pairing shipped as a `MapDocument{map, free}` struct in MapJson with toJson/mapDocumentFromJson overloads and saveMapFile/loadMapDocumentFile beside the untouched TileMap-only signatures, which now always write schema 2 with an all-pinned free section.
The loader accepts schema 1 and 2, a v2 document without the free section is rejected, and a stray free section on a v1 document is read rather than ignored.
assets/maps/demo.json was re-exported as v2 (schema 2 plus an all-pinned free block) and map_assets_validate stays green; the eight new MapJson tests bring the tilemap suite to 45.

## 12: runtime keyboard under the raylib backend

Task 1b's investigation found that the raylib input backend declares keyboard=false and emits no key events, so tilemap_demo's arrow movement and every editor fast path are dead in a real run.
Investigate first how the interactive apps (game, life, sudoku, atlas_editor) receive keyboard under raylib — an app-lib/gfx-side source such as the WindowedSession pipeline or game's KeyboardSource, or an unimplemented backend path — and follow the repo's intended path rather than inventing one.
If the intended path is app-lib or gfx-side, wire map_editor's KeyboardSystem and tilemap_demo to consume it (footprint: both apps plus src/libs/app for a genuinely needed additive helper).
If the intended path is the input backend, implement key events there additively with keyboard=true, using raylib's key-pressed queue or edge tracking that cannot drop sub-frame events, without replicating the mouse state-diff defect and keeping backend tests green.
Either way tilemap_demo's arrows and Escape must work, as must the editor fast paths (1-7, E/Q, B, L, U/R, S, G, V, Tab, Escape, F5).
Verification: xdotool key events under Xvfb move the demo player and drive editor paint/undo/menu-close, or if key events cannot reach raylib under Xvfb, an explicit statement plus null/replay-backend verification and a written event-path trace.
Amendments at shipping (2026-08-10): the investigation found no app-lib or gfx-side keyboard source anywhere — the game/life/atlas_editor pipelines all consume the same IInputBackend that declared keyboard=false — so the intended path is the input backend, and key events landed there additively.
Presses drain raylib's GetKeyPressed queue (which holds sub-frame presses, so nothing can drop) with a repeat flag from a held-key table, releases come from IsKeyDown edges over that table, modifiers are sampled at event time, and capabilities now claim keyboard=true; 93 of the 105 Key values map (IntlBackslash has no raylib code).
RaylibWindow now calls SetExitKey(KEY_NULL) so applications own Escape instead of raylib closing the window on it.
Tab additionally became the editor's Map/Tiles/Characters cycle (the task 7 gap), U/R/S route to the sheet undo, redo, and save while a workspace view is active, and xdotool verified: demo arrows plus Escape-quit, editor brush digits, E/U, S (file mtime), V, G, Tab, Escape closing menus and the file dialog without quitting, and a full file name typed into the Save As field with Backspace editing and Return submit.

## 13: map palette picker

The artist picks the map's two 1-bit palette colors — ink and paper — with a full-range color picker; MapHeader already carries Rgb ink and Rgb paper and the JSON schema serializes them, while today both apps render hardcoded constants.
Rendering first (footprint src/apps/map_editor plus src/apps/tilemap_demo): sheets bake in white ink and map sprites draw with tint = the header's ink color (Shade keeps its black tint); loaded assets/tiles PNGs either tint-multiply, if drawn in white or light gray, or re-color on load by mapping every opaque pixel to white — whichever keeps existing PNGs looking right, reported.
clear() uses the header's paper color, and the task 10 checkerboard shades plus any paper-relative chrome derive from the header palette so light maps stay readable; ui panel and menu theme colors stay fixed.
Palette changes take effect live with no texture re-bake, or a single cheap re-bake on change (implementer's call, reported).
tilemap_demo renders with the loaded map's header palette, and its built-in demo map keeps the current colors as its header values.
Picker ui: Map > Palette... opens a modal dialog under the file dialog's modality rules — two swatches (ink, paper) with the active one selectable, a hue slider, a saturation/value 2D square rendered as a generated texture regenerated on hue change with click or drag picking, R/G/B readouts, and a #rrggbb hex field; live preview recolors the map behind the dialog; Apply commits one undoable header edit through the command path and Cancel restores the colors from before the dialog opened.
If the SV square proves awkward, six sliders (R, G, B per swatch) plus the hex field still count as full-range, but the SV square comes first and the outcome is reported.
Verification under Xvfb (mouse-only unless task 12 has landed): drag in the SV square and hue slider with the map recoloring live behind the dialog, Apply then save and assert the JSON header ink and paper match the picked values exactly, reload to confirm persistence, capture the Cancel path restoring prior colors, and undo after Apply reverting; standard bars throughout.

Amendments at shipping (2026-08-10): loaded assets/tiles PNGs re-color on load — every opaque pixel maps to white with alpha kept — rather than trusting the file's baked color, so pre-task-13 sheets saved with the old gray-green ink render identically once tinted.
Palette changes are tint-only with zero re-bake: the sheet textures stay white-baked and only the per-draw tint and clear color change.
The picker keeps hue, saturation, and value as its working state while dragging and recomputes them from the swatch only on open, swatch switch, or hex entry, so integer rounding in the conversions never makes a drag drift.
The SV square shipped as specified: a 128x64 texture regenerated on hue change, drawn by the ui system after the panel paint using the widget rect the layout reports, with press-and-drag hit-testing done against that rect.
The hex field accepts rrggbb without the leading '#', because the raylib key path delivers shift+3 as a plain '3', and the field text still displays the '#rrggbb' form.
Fresh maps from makeEditorState and File > New now carry ink 214,224,216 and paper 12,14,16 in the header, and DemoMap plus a re-exported assets/maps/demo.json carry the same values, so every default view keeps the pre-task-13 look while a default-header v1/v2 map (black on white) renders honestly by its own palette.
The checker shades and void derive arithmetically from paper (a 12/255 and 24/255 step toward white on dark paper, mirrored toward black on light paper, void 143/255 toward black on dark and 64/255 on light), while the ghost and free-mark chrome picks a light or dark constant set by paper luminance.
The Tiles and Characters workspaces keep the fixed dark clear color, since their margins are editor chrome, not map presentation.
A `modalOpen` helper now guards pointer gestures, fast paths, F5, and Tab for both the file and palette dialogs, and Escape cancels the palette dialog exactly like its Cancel button.
Verification passed live under Xvfb: SV-square and hue drags recolored the map behind the dialog mid-drag, typed hex values 3366cc/eeddaa landed exactly as [51,102,204]/[238,221,170] in the saved JSON, a relaunch rendered them, Cancel and Escape restored the prior colors, one undo after Apply reverted both colors together with redo restoring them, an outside click neither painted (wall brush over floor stayed floor in the JSON) nor closed the dialog, and tilemap_demo rendered the picked palette from the file while the built-in map kept the classic colors.

## 14: developer console in tilemap_demo and map_editor

Wire the repo's developer console (src/libs/console) into tilemap_demo — the Wakewater game stand-in — and, if the pattern transfers cheaply, into map_editor as well.
This task depends on task 12, because a console needs typing, so it runs after runtime keyboard lands.
Study the existing mounts first: the console lib serves nine apps, life is likely the smallest example (ConsolePicture overlay in its RenderSystem) and game the fullest (ConsoleMount/ConsoleScene/ConsoleSink/ConsoleGatedSink, Typing, IConsoleCommands implementations, OptionsConsoleControls, SnapshotCommands with a JSON snapshot store); follow the repo's canonical mount pattern rather than hand-rolling one.
tilemap_demo (required): the console toggles with the repo's conventional key (matching what the other apps bind), overlays the top portion of the canvas, captures keyboard while open with movement suppressed, and closes with the same key or Escape per convention.
Commands via IConsoleCommands, v1 set: help, map <path> (load a map file with errors printed to the console), tp <column> <row> (teleport clamped with a printed warning on unwalkable targets), pos, palette <ink|paper> <#rrggbb> (only if task 13 landed), and quit.
If the SnapshotCommands/ISnapshotStore pattern fits without inventing state serialization, wire snapshot save/load the way game does; if it drags in more machinery than the demo is worth, skip it and record why.
Footprint: src/apps/tilemap_demo/** plus linking antwika::console.
map_editor (if cheap after the demo wiring, else recorded as skipped with reasons): same mount pattern with help, open <path>, save [<path>], generate [seed], validate, scale <2|3|4> (if task 9 landed), and palette; while the console is open it owns all keys and never fights the ui text fields.
Verification under Xvfb with post-task-12 keyboard: capture the open console overlay with prompt, type a tp command and capture the moved player, type map with a bad path and capture the error in the history, close and confirm movement keys work again; for the editor, open, type save, and confirm the file mtime changed; standard bars, and the console lib's own tests stay green.

Amendments at shipping (2026-08-10): the study found that ConsoleMount hardwires SnapshotCommands as its whole command table and no app passes custom commands, so both apps assemble the mount's own inner piece — the lib's ConsoleSink with ConsoleState, ConsoleScene, FixedConsoleControls, InputFold, and ConsolePicture — around an app IConsoleCommands, with the console lib untouched.
The snapshot save/load pattern was skipped in both apps: IJsonSnapshotStore requires a magic, a version, and a migration chain, which is more machinery than a demo whose whole mutable state is one player cell and a map path is worth, and the editor already owns richer persistence through its map files.
The conventional keys are FixedConsoleControls' defaults — Grave toggles, Enter executes — and since the lib itself never closes on Escape, both apps close the console app-side on an Escape press while it shows, so Escape never quits through an open console.
Both direct-loop apps feed the sink by hand: one engine.tick TickEvent per frame for the slide animation and repaint, then every polled input event encoded through InputEventCodec, which is exactly what the engine-loop apps' dispatcher does.
The overlay is window-sized (life's precedent, whose "canvas" is the window) and painted on the window renderer above the viewport, because at the apps' small canvases the scene's eight history rows overflow the half-canvas sheet and render nothing.
tilemap_demo shipped help, map, tp, pos, palette, and quit; tp clamps into bounds and refuses an unwalkable target with a printed warning, map prints load errors into the history, palette reuses task 13's semantics, and quit routes through the sink's stop path into window close.
map_editor proved cheap after the demo wiring and shipped an EditorConsoleSystem scheduled first in the input phase, with help, open, save [path], generate [seed] (seeding the store counter), validate (reporting the finding count), scale <2|3|4> (a store-carried pending scale the ui system applies, rebuilding the overlay at the new window size), palette (one undoable header edit), and quit.
While the console shows, the editor's KeyboardSystem returns before reading any key, so the console owns all keys and never fights the ui text fields, and pointer presses and wheel scrolls under the console sheet are swallowed following ConsoleGatedSink's semantics; the hex commands accept bare rrggbb because shift+3 still types '3'.
Verification under Xvfb passed: captures show the demo console overlay with prompt and help, a tp 10 8 moving the player with arrows dead while open and alive after close, a bad map path's error in the history (typed with a dash-named file since the layout delivers no '/'), Escape closing the console with the app surviving, and quit exiting cleanly; the editor captures show the console over the live map, save creating the file and a later save changing its mtime, validate/generate/palette/scale acting with the map recoloring and the window resizing live, undo reverting the console's palette edit after close, and quit closing the editor.
All 6633 display-bound tests plus the 80 headless conformance tests stay green, including the console lib's suites.

## 15: fullscreen support

Fullscreen toggles by F10 or View > Fullscreen in map_editor, by F10 alone in tilemap_demo, persists in the editor's config file, and is applied at startup.
The gfx::IWindow fullscreen API turned out to already exist at the base commit — pure-virtual setFullscreen/isFullscreen with a WindowDesc.fullscreen field, the null backend recording the flag, and game's F10 event source — so the backend work here is the mechanism switch, not an API addition.
backends/raylib now toggles through ToggleBorderlessWindowed and answers isFullscreen from IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) instead of exclusive ToggleFullscreen, keeping the desktop resolution and sane alt-tab; the gfx conformance suites stay green over the new mechanism.
The editor's View menu gained "Fullscreen  F10" with a "*" marking the active state like the UI Scale entries, and F10 rides the keyboard fast path through a store-carried pending toggle the ui system applies, guarded like F5 so it is inert while a modal dialog is open (and while the console is open, since the console owns all keys).
On toggle the task 9 resize machinery is reused: after setFullscreen the actual window size is queried and the ViewportRenderer transform rebuilt, and a store-held window size lets the task 14 console overlay rebuild itself at the new dimensions.
Selecting a UI Scale while fullscreen leaves fullscreen first and then applies the windowed scale, rather than sitting inert — the chosen and reported call.
The config file gained an optional boolean "fullscreen" member written on every toggle and scale change and applied after window creation, before the ViewportRenderer is built, so a persisted fullscreen start computes its transform from the real monitor-sized window; files without the member load as windowed.
tilemap_demo handles F10 in its loop with the same rebuild plus a console overlay rebuild at the new window size.
Verification under the 1920x1080 Xvfb root: F10 moved the editor from 1440x810 at 240,135 to 1920x1080 at 0,0 with the capture filling the root at a crisp 4x, a wall painted at cell 5,4 while fullscreen landed exactly there in the saved JSON, F10 restored the windowed geometry, UI Scale 2x chosen while fullscreen left fullscreen into a 960x540 window with the config recording uiScale 2 and fullscreen false, a relaunch after quitting fullscreen started at 1920x1080 from the persisted flag, and the demo toggled 1280x720 to 1920x1080 and back with arrows alive throughout.

## 16: drag-resizable windows

The editor and demo windows resize by dragging, with the viewport adapting continuously while the canvases stay 480x270 and 320x180.
gfx::WindowDesc already carried a `resizable` flag at the base commit, applied by backends/raylib through FLAG_WINDOW_RESIZABLE after InitWindow (raylib accepts the state change post-init, so no SetConfigFlags reordering was needed) and ignored by the null backend, so no gfx or backend change shipped — both apps simply opt in with resizable = true.
Adaptation uses the event path: the raylib backend already emits a gfx Resized window event whenever the polled size changes, but app::closeRequestedOn drained and discarded it, so both main loops now drain backend->pollEvent() themselves, treating CloseRequested as before and answering Resized with the task 15 path — view.resize(size) plus the console overlay rebuild (through the store-held window size in the editor, directly in the demo).
A behavior divergence is recorded rather than "fixed": viewportFor letterboxes with an exact reduced rational scale, not the largest integer multiple, so a 1000x700 window renders the editor canvas at 25/12 with top and bottom bars, and a window smaller than the canvas scales below 1x, shrinking to fit with the aspect kept instead of clamping at 1x and cropping.
That contract is shared by every app and pinned by the gfx tests, hit-testing inverts it exactly at any size, and the sub-canvas capture stays coherent, so it was kept as-is.
A manual drag never writes the config file — the persisted uiScale remains the last explicitly chosen scale — and the UI Scale entries still snap to exact window sizes, which required the setUiScale early-return to also compare the actual window size so re-choosing the persisted scale after a drag still restores its exact geometry.
Verification under Xvfb: xdotool windowsize drove 1000x700, 1500x900, 700x400, and a sub-canvas 400x220 with captures showing crisp aspect-true letterboxing and no stretching, a wall painted at cell 7,3 in the 1000x700 window landed exactly there in the saved JSON, the config stayed untouched by drags, UI Scale 3x afterwards snapped back to 1440x810, F10 still entered and left 1920x1080, and the demo resized through 1000x700 and 640x500 with arrows moving the player throughout.

## 17: generate-every-second-press bug

User report: "Only every second press on Map > Generate does generate the map."
Reproduction under Xvfb before any theory: six menu presses on a fresh map with the JSON saved and hashed after each showed changes on presses 2, 4, and 6 only, and the same alternation held for the G key and the panel Generate button, which exonerated the menu-dispatch and pending-plumbing candidates immediately since those are path-specific.
The flushed log then overturned the seed-parity-contradiction candidate too: every press logged "generated with seed N" with no failures — generation ran and applied every time, but consecutive seeds produced byte-identical maps in pairs.
Root cause: makeWave seeded its xorshift32 stream with `seed | 1U`, which collapses seeds 2k and 2k+1 onto the same nonzero state; with the rest of the solve fully deterministic, seeds 2 and 3 (4 and 5, and so on) yield the same map, so the incrementing per-press counter visibly regenerated only every second press.
Fix: the stream now starts from `scrambled(seed)`, a splitmix-style finalizer (add golden-ratio constant, two xor-shift-multiply rounds) with a zero-state remap to one, so every practical seed gets a distinct nonzero xorshift32 state; the coordinator's fallback idea of retrying the next seed on contradiction was not needed because no contradictions were occurring.
Verified after the fix: six consecutive Map > Generate presses produced six distinct terrain hashes with a pre-painted pinned water cell and the pinned wall border verbatim in every snapshot, two G presses and two panel presses each changed the map identically, one undo reverted exactly the last Generate (the saved JSON matched the previous press's snapshot), and a constructed contradiction — pinned water beside pinned cliff, an incompatible adjacency — failed in about a second with "generate failed (seed 11)" in the console, the yellow panel notice, a live editor, and a map untouched apart from the two painted pins.

## 18: yellow hover outline

The cell under the pointer now carries a thin outline in the theme's focus-ring yellow, so the artist always sees exactly which cell a click will hit.
Map view: a new drawHover in OverlayDraw outlines state.hovered through the same cellRectF/cellOrigin path as the selection outline, so it follows the elevation lift; the color is read from ui::Theme{}.focusRing rather than a new constant, and the thickness is one canvas pixel at 1x, scaling with the camera zoom (clamped to one pixel below 1x).
MapRenderSystem draws it last in the render phase — above tiles, markers, and the validator overlay, and below everything the ui phase paints — and gates it on the pointer actually being over the map viewport: no outline while the pointer sits over the panel or the menu bar, while a modal dialog is open, or under a visible console sheet.
The auto-extend ghost cell shares no code with the hover outline — it keeps its own paper-derived gray fill and edge from task 13 — so nothing needed differentiating, and the selected-entity outline stays white so hover and selection read apart when they coincide.
The tile and character pixel workspaces already had an atlas_editor-style hover affordance in the same yellow (a translucent fill at the hovered pixel), which is reinforced rather than replaced: a shared drawPixelOutline helper in SheetWorkspace now draws an opaque one-canvas-pixel focus-yellow outline just inside the magnified pixel in both workspaces, over the existing fill.
Verified under Xvfb with captures: the outline on a hovered cell at 1x, the outline riding a two-step lifted plateau, a thicker outline on the correct cell at 2x camera zoom, zero focus-yellow pixels in the map area while hovering the panel and while the palette dialog is open (pixel-scan assertions), and the crisp pixel outline in both the tile and character workspaces.

## 19: pointer hint line

A single line of small-face muted text sits two pixels off the bottom-left of the canvas in every view, describing what the pointer points at, drawn by the ui system after the panel paint and rebuilt only when the hovered target changes.
The cache key (view, hovered widget, pointer position, modality, undo depth) lives beside the string in the ui system; the undo depth rides along so an edit under a still pointer — painting, raising, bridging — refreshes the hint without waiting for movement.
Map view hints render most-specific-first: "cell 5,4  floor h=0" with appended annotations for bridge, non-full light ("light 160"), a free cell, the water flags (deadly, swimmable, current with its direction), and every entity on the cell as "door-1 transition"; the auto-extend ghost says "paint to extend the map" and the void says nothing.
Widget hints come from ui::Interactions.hovered, which the ui library's hover machinery already exposes as a plain WidgetId — enough to map the editor's whole widget roster (brushes, cell tools, entity tools, fields, menu titles and every dropdown entry, file-dialog rows by their listed name, palette controls, and character rows as "select character player") without touching HoverTargets, so no limitation needed reporting.
Tiles view reuses the task 7 slot naming as "mask 5  px 10,12" with an "ink" suffix on inked pixels, and the characters view reads "walk_up frame 1  px 30,19"; the old bottom-left workspace labels and ink indicator are removed so slotLabelAt and rowNameOf feed exactly one presentation, and the workspace draw functions dropped their now-unused bitmap parameter.
While a modal dialog is open the hint describes only the dialog's own widgets ("apply the palette as one undoable edit") and is otherwise empty, never leaking map hints from underneath; when the pointer sits under a visible console sheet the hint empties too, and the line itself would hide if the sheet ever reached it.
Verified under Xvfb with captures: the annotated cell hint over a bridge-plus-light cell carrying a transition, the ghost hint past the south edge, the Generate panel hint, the tile-slot and character-frame hints, a palette-dialog widget hint, and zero muted-text pixels in the hint strip both over the void and with the pointer parked under the open console.

## 20: disconnected-tiles rendering bug

User report: the 16x16 unit tiles are not visually connected — it looks like only the lower-right sprite is ever drawn, the other three sprite slots appear seemingly empty, and terrain is not continuous.
The before-fix capture confirmed the geometry: addSurface drew ONE 8x8 sprite per dual corner at (dc*16-8, dr*16-8), which covers only the quadrant north-west of the corner — the SE quadrant of one cell — so 8x8 sprites at 16-pixel spacing painted exactly a quarter of the plane and three quadrants of every cell stayed empty.
Fix: the standard dual-grid display-tile scheme — a surface sprite is now a 16x16 display tile drawn centered on its corner (the draw position was already the correct top-left for that), giving full plane coverage and edges the artist controls directly.
The mirrored-8x8-quadrants alternative was rejected: raylib's DrawTexturePro could flip via negative source rects, but the IRenderer/ViewportRenderer contract and every backend and conformance suite assume positive rectangles, so it would be an invasive gfx-contract change for flip-dependent art, against an app-and-lib-local layout change with native 1x pixels.
The sheet is now 96x64: the 4x4 mask grid of 16x16 display tiles fills the left 64x64, the three 16x16 variant tiles plus a spare sit top-right, and the four 8x8 specials (band, rim, bridge deck, shade) sit beneath them — laid out wide rather than as a 64x88 stack because the workspace area is 320x260 and the wide sheet fits at a 3x magnification where the tall one fits only at 2x.
sheetSource now returns 16x16 rects for surfaces and 8x8 for specials, and both apps size the destination rect from the source instead of a hardcoded 8x8; DrawPlan itself is unchanged apart from that, since faces, bridges, and shades were always per-half-tile draws.
The bilinear placeholder coverage extends to 16x16 with the corner at the tile center (weights over x/15, y/15) and the terrain patterns tile per 8x8 quadrant; the editor's generator now carries the full slot set so both apps' C++ and the regenerated scripts/generate_placeholder_tiles.py produce identical coverage, and assets/tiles was regenerated.
The workspace runs at 3x with the pixel-grid gate lowered to 3, strong guides on the 16-pixel display-tile boundaries and fainter 8-pixel quadrant guides inside them, relabeled slots feeding the hint line, and a loaded 32x48 PNG logs "legacy sheet layout, redraw needed" and falls back to the placeholder.
Verified under Xvfb: the before capture shows the scattered quadrants, the after captures show a connected wall border and slab, a coherent water pool boundary, cliff bands flush under a raised plateau lip, and the demo's built-in map reading as one continuous scene; a pixel scan found zero empty 8x8 quadrants inside painted wall and floor regions (minimum 288 and 18 ink pixels per quadrant), the task 7 restart round-trip passed on the 96x64 sheet with the edited art rendering after relaunch, and the legacy-sheet warning plus placeholder fallback were exercised live.
docs/TILE_SHEETS.md was rewritten for the new convention and docs/GUIDE.md's tiles section updated.

## 21: eight interior variants and connected pipes

Interior variants grow from three slots to seven alternates beside the mask 15 base, all 16x16 display tiles, and the wall interiors now read as a jumble of connected pipes.
The sheet grows to 96x80: the 4x4 mask block keeps the left 64x64, the seven variants and water frame B stack two wide and four tall in the right column, and the four 8x8 specials move to a bottom strip at 0,64 — a static layout that fits the tile workspace at the existing 3x magnification with no sheet panning (96x80 at 3x is 288x240 inside the 320x260 area).
sheetSource maps variants 1 through 7 to the column and variant 8 to water frame B, with anything larger falling back to the base; the scatter draws bucket = hash % 14, giving the base exactly half the tiles and each variant a fourteenth, deterministic as before.
Water keeps its animation on the base only — a water tile whose scatter lands on a variant stays static, and one on the base cycles to frame B on the clock — the simplest honest reading, since animating every variant would need seven more frame slots.
The connection convention is documented in TILE_SHEETS.md: two-pixel pipes cross tile edges only at the edge midpoints (pixels 7 and 8), run at least four pixels straight inward, and unused midpoints stay clear so an arriving neighbour pipe reads as entering the wall mass.
The wall placeholder's base became the pipe cross touching all four midpoints — at half of all tiles it is what makes long runs certain — and the variants are the straight horizontal, straight vertical, north-east elbow, south-west elbow, west-east-south T, valve box, and four-way tank, over a sparse-dot backdrop replacing the old interior checker; floors gained quiet unconnected details (seams, rivets, grate, extra dots, corner seam, hatch) and the other terrains phase-shift their pattern per variant.
Both C++ placeholder generators and scripts/generate_placeholder_tiles.py carry identical math, assets/tiles was regenerated, the workspace labels and hint line name the new slots ("variant 5"), and the 96x64 layout from task 20 joined 32x48 in the legacy redraw-needed fallback.
Verified under Xvfb: an 11x7 painted wall slab renders as a visibly connected mechanical jumble with valve boxes and a tank, a pixel-check found horizontal runs crossing six tile boundaries and vertical runs crossing four with ink on both sides of every shared midpoint, the floor field shows scattered detail variety, two captures of the same map were pixel-identical, the workspace edited a variant slot with the hint naming it and the edit surviving the restart round-trip at 96x80, the legacy 96x64 sheet warned and fell back, and the demo rendered the identical pipe jumble.

## 22: character eraser and ink consistency

User report: left-click draws white, right-click draws black with no way to erase to transparency, and the example character is off-white with no way to match its color.
The confirmed root cause sat below the diagnosed one: gfx::Color defaults alpha to 255, so setSheetPixel's "clear" color `Color{}` painted opaque black in BOTH pixel workspaces — invisible over the dark map for tiles, glaring against the character checkerboard — and a later load would resurrect those black pixels as ink through the opaque-to-white normalization.
The one-line fix clears with an explicit zero alpha, restoring the strict ink-or-transparent model everywhere; no black-painting path remains.
The second half matched the diagnosis: the New-character silhouette baked the old fixed 214/224/216 ink while strokes baked pure white, so the template now writes white, character loading normalizes any opaque pixel to white exactly as task 13 did for tiles (the user's off-white player.png becomes consistent with no action), and characters render tinted by the map's ink color everywhere — the workspace canvas, the animated preview, and the demo's player sprite, which previously drew with a fixed white tint.
The hint line's "ink" suffix now applies to the character workspace too, and the guide's characters section states the left-inks/right-erases model and the ink-color rendering.
Verified under Xvfb: a right-click over an inked body pixel now shows the checkerboard through the hole (before the fix the same click left an opaque black pixel, sampled as 0,0,0), a paint-and-save round-trip left player.png with every opaque pixel at 255/255/255 by byte assertion, and after a console `palette ink 22ddff` the workspace canvas, the animated preview, and the demo's player all sampled exactly 34/221/255.
The user's original player.png was restored after the test edits, since load-time normalization makes rewriting it unnecessary.

## 23: artist-editable adjacency rules and weights

The WFC adjacency table and generation weights moved out of Generate.cpp into GenerationRules, loaded from <tiles>/rules.json (honoring --tiles) with the old table kept as the compiled-in default.
The schema, documented in TILE_SHEETS.md, is a weights object per generatable terrain plus an adjacency array of symmetric two-name pairs; validation rejects unknown names, non-positive weights, and malformed pairs, and a missing or corrupt file logs a warning and falls back to the defaults.
Stair stays artist-only in the domain exactly as before, with a fixed weight.
Map > Rules... opens a modal under the file-dialog modality rules with the 6x6 symmetric pair matrix (self-pairs included, one click flips both cells) and integer +/- steppers for the five generatable weights, labeled "tileset-level, not part of map undo"; Apply writes rules.json and swaps the live rules so the next Generate uses them, Cancel and Escape discard.
scripts/generate_placeholder_tiles.py now regenerates a defaults-matching rules.json beside the sheets, and assets/tiles ships one.
Verified under Xvfb: toggling wall-floor off and applying produced a generated map whose saved JSON had zero wall-floor 4-adjacencies, raising water's weight from 2 to 8 took the water count across two seeds from 2 to 7 cells, a corrupt rules.json logged the warning and generation showed wall-floor adjacencies again (defaults in effect), and a relaunch after Apply still generated with the saved rules.

## 24: per-variant edge connectors

Interior variants 1 through 7 can declare which of their four edges carry a connector, stored in the tiles.json sidecar as N/E/S/W letter strings per terrain and variant; unspecified variants default to all four edges, and the base is implicitly all-connected.
The autotile library gained a Connectors header and a buildDrawPlan overload taking per-terrain connectors, with the plain overload passing all-connected defaults — a captured all-wall map rendered pixel-identically to the pre-change reference with no sidecar config present, and the demo keeps the plain overload.
The scatter is edge-aware and deterministic: variants are chosen row-major so each choice sees its west and north neighbours' facing edges and must match both, connector to connector and blank to blank; the status-quo hash pick is tried first (which is what preserves default behavior bit-for-bit), and on a miss the fallback prefers both-edge matches (base first, else the hash picks among them by id order), then west-only, then north-only, then the base — the rule documented in TILE_SHEETS.md.
Edges facing non-interior tiles stay unconstrained, water frame B stays base-only, and the art-matches-declaration duty is documented as the artist's responsibility.
The workspace shows four edge markers at the hovered variant tile's edge midpoints (yellow for connector, dim for blank), a click on a two-pixel midpoint hotspot toggles one (dragging through still paints), Save Sheet writes the section, the hint line reports "variant 1  px 79,7  ink  edges E,W", and the connector state rides the hint cache key so a toggle refreshes the line immediately.
Superseded by task 26: the toggle now requires ctrl+click over an enlarged midpoint zone, plain clicks paint everywhere on the tile, and the markers render as outlines.
Verified under Xvfb with a test sidecar declaring truthful contacts (straight-H E+W, straight-V N+S, the elbows, the T, the valve E+W, and the tank with no edges): a Python replica of the hash and chooser predicted every interior corner's variant, every shared edge matched by declaration, and pixel checks at all shared midpoints inside the wall slab showed ink exactly where connectors meet and blank where blanks meet with zero mismatches — the no-edge tank simply never enters the constrained interior, which is the mechanism keeping pipe stubs out of blank-edged neighbours.
Two captures of the same map were pixel-identical, the E-edge toggle saved as "W" in tiles.json and survived a relaunch, and the truthful-contact interior visibly collapses toward the all-connected cross, an honest consequence of edge matching that richer all-edge art relieves.
The shipped assets/tiles sidecar carries no connectors section, so defaults apply out of the box.

## 25: two-color drawing with palette baking

Both pixel workspaces now draw with two colors plus transparency: art declares ink-class or paper-class pixels and the actual colors keep coming from the map palette, so every map recolors the art as before.
Storage stays ordinary PNGs — opaque white 255 is ink, opaque mid-gray 128 is paper, zero alpha is transparent — and load-time normalization classes any opaque pixel by a 192 luminance threshold, so existing pure-white sheets load as all-ink and off-white legacies still normalize to ink; both placeholder generators keep emitting ink-only art and remain byte-identical to each other, with assets/tiles unchanged.
Rendering switched from tint-at-draw to bake-at-palette: sheet and character textures are composed with the palette's actual ink and paper applied to their classes, re-baked whenever the palette changes — Apply, the picker's live preview, and the console palette command — while Shade keeps its black tint and every draw call now tints white.
No rebake throttle was needed: seven tiny bitmaps re-bake per palette change and the picker drag stayed fluid, recoloring both classes mid-drag.
tilemap_demo bakes the same way, keeping the placeholder and player source bitmaps and rebuilding textures when the map header palette changes.
The artist picks the active draw color through two panel swatches showing the live palette colors (or the C key), left-click paints that class, right-click erases to transparency, the hovered-pixel hint suffix reads ink, paper, or blank plus the active "drawing" color, and the workspace canvas shows classes in the live palette so what you draw is what the map shows.
Verified under Xvfb: ink and paper strokes rendered in the palette's cyan and near-black over the checkerboard, the saved wall.png byte-asserted exactly the 255-white and 128-gray opaque set with correct alphas, the classes round-tripped a relaunch with the hint reading "paper", 27 base wall tiles sampled both classes in their palette roles and again after a console palette change to new ink and paper values, the picker drag recolored both classes live behind the dialog, right-click erased both classes back to the checkerboard, a paper detail painted on the player rendered in the paper color in the demo beside the ink body, and the all-ink legacy map render stayed pixel-identical to the pre-change reference.
The task 22 rule that characters tint with the ink color at draw time is superseded by the palette bake, which colors both classes.

## 26: ctrl+click edge toggles and outline markers

User report: the edge markers' click hotspots blocked normal pixel drawing underneath them, and the filled markers hid the artwork.
Edge connector toggles now require ctrl+left-click; a plain left or right click anywhere on a variant tile — markers included — paints or erases exactly like the rest of the tile.
The raylib input backend declared a modifiers field on pointer events but never filled it, so it now samples the modifier state on pointer presses and releases exactly as the task 12 key path does, additively, with all backend suites green.
Since the toggle no longer competes with drawing, the hotspot grew from the 2x1 midpoint pixels to a six-pixels-along, two-deep midpoint zone per edge.
Markers render as one-canvas-pixel outlines with no fill — yellow for connected, dim for blank — plus a one-pixel black inner contrast line, which shipped because a bare yellow outline over light ink or paper art is ambiguous; the artwork stays visible through the hollow center.
The hint line appends "ctrl+click toggles" over variant tiles, and the guide and TILE_SHEETS.md describe the new gesture; task 24's section carries a supersession note.
Verified under Xvfb: a plain click on the west marker zone painted a paper pixel there (stored as 128-gray in the saved PNG) and left the sidecar connector-free, a ctrl+click in the same zone toggled the west edge off (sidecar saved wall variant 1 as "NES") without painting (the clicked pixel stayed 255-white ink), the hint read "variant 1  px 65,7  ink  drawing paper  edges N,E,S  ctrl+click toggles", and marker-interior samples showed the ink-colored artwork rather than any marker fill, with the dim blank-edge outline distinct from the yellow connected ones in the capture.

## 27: quadrant libraries with 8px edge matching

Per terrain, the artist may draw a library of 8x8 quadrant sprites with per-slot connector flags; when the sidecar declares at least one quadrant slot for a terrain (the activation trigger), that terrain's interior assembles from the library on a uniform 8-pixel lattice with edge matching instead of scattering 16x16 variants.
The sixteen library slots reuse the previously spare sheet rows — slots 0 to 7 at (32,64) and 8 to 15 at (0,72) — so the sheet stays 96x80, the workspace needs no panning, and legibility is unchanged.
The autotile library gained TilePiece::Quadrant with quadrantSource, quadrant fields on SheetConnectors (edge bits plus a declared-slot mask), and an addQuadrantSurfaces path: interior full-mask tiles become four 8x8 draws on the lattice (which ignores display-tile boundaries), partial masks keep their surface pieces, and the row-major choice matches west and north facing edges with the lowest declared slot as favored base and the section 24 fallback chain; edges facing non-interior tiles stay unconstrained.
Water frame-B animation stays variant-path-only — a quadrant water interior does not animate — the simplest honest choice.
The sidecar grew a per-terrain "quadrants" object mapping slot ids to edge-letter strings; ctrl+click on a quadrant slot's four-along, two-deep edge zone declares the slot and toggles the edge with the section 26 outline markers scaled down, ctrl+right-click removes a slot from the library, declared slots show a corner dot, and hints read "quadrant 0  px 32,66  ink  drawing ink  edges N,E,S  ctrl+click toggles" or "not in library  ctrl+click adds".
The shipped wall sheet carries a demonstration library (cross, straights, elbows, tee, west cap, plain block) drawn identically by both C++ generators and the script, but the shipped sidecar declares nothing, so shipped behavior is unchanged; the activating configuration is given as a documented example in TILE_SHEETS.md.
The load fix along the way: loadConnectorsFile no longer early-returns when the sidecar has quadrants but no 16x16 connectors section — caught because the first slab capture rendered the variant path and the pixel checks refused it.
Verified under Xvfb: with no sidecar config the render is pixel-identical to a stash-built pre-change reference; with the test library active, a Python replica of the lattice chooser predicted all 240 interior quadrants, pixel checks at every shared 8px edge (including 120 boundaries interior to display tiles) showed connector-meets-connector and blank-meets-blank with zero mismatches and 385 connector crossings — visibly denser than the 16px network; two captures were identical; plain click in a marker zone painted (255-white byte-asserted in the PNG) while ctrl+click toggled the sidecar to "NES" without painting (alpha stayed zero); a floor strip rendered identically with and without the wall library present, proving variant terrains untouched; and a restart round-tripped both the painted quadrant art and the connector state.

## 28: socket-based tilesets with layers, decor, and animation

The fixed 96x80 sheet system — corner-mask grid, variant column, quadrant slots, and letter-coded connectors — is replaced wholesale by artist-authored tilesets: named, growable libraries of 8x8 sprites, each bound to one terrain, stored as `assets/tilesets/<name>/` directories of a JSON sidecar plus one class-coded PNG per layer.
Adjacency is decided by named edge sockets (equal names may touch) with two reserved names: `edge` marks a side facing out of the terrain region, so borders emerge from the socket system on a 1:1 un-offset lattice where each painted cell owns its own 2x2 sprites, and `open` marks a decor edge facing emptiness.
Tilesets carry layers — layer 0 tiles the region, higher layers are decor scattered deterministically by a per-layer density gate wherever the decor sprite's base-sprite allowlist and decor-decor sockets allow — and each sprite may carry up to four animation frames cycled on the global clock, generalizing the old water special case to every sprite.
Maps bind a tileset per terrain in the header (schema v3; older maps load unbound and fall back to `default-<terrain>`, then to a compiled placeholder), applied as one undoable edit through Map > Tilesets....
The work landed as a new tested `tileset` lib (model, byte-stable JSON codec, layer-PNG store, atlas bake; 91 tests), map schema v3 in `tilemap` (53 tests), a rewritten autotile assembler keeping the per-level, face, bridge, shade, and cutaway pipeline, a rebuilt Tiles-view workspace (zoomed sprite editor with colored socket bands, paged library grid with socket ticks, frame strip with anim preview, Draw/Sock/Decr tools, per-tileset snapshot undo), New Tileset and Bindings dialogs, a regenerated placeholder generator with byte-exact `--check` CI, and TILESETS.md replacing TILE_SHEETS.md.
Verified under Xvfb with synthetic input: map and demo render the generated default tilesets with socket-matched borders; pixel strokes, socket add/apply/clear, frame-2 creation, sprite and tileset creation all undo correctly; a new wall tileset saved, survived an editor restart, rebound onto the demo map through the bindings dialog (visibly restyling every wall region), and reverted with a single undo; decor motifs scatter on allowlisted floor sprites; water animates at runtime across timed captures; the demo app renders and moves the player; and the repo tree stayed pristine throughout.
Known quirks parked for hardening: an ink-on-ink stroke still marks the tileset dirty and pushes an undo step, and quitting discards unsaved tileset edits without a prompt.

## 29: blank New, four frames, reverse view cycling, rebindable hotkeys

Four artist-workflow refinements shipped as one batch.
File > New (and the no-`--map` startup) now yields a truly blank slate: a 20x11 all-floor, height-zero, fully pinned field with no wall border, no entities, default palette, empty tileset bindings, cleared undo history, and a reset camera — so Generate does nothing until the artist marks free cells with brush 7.
The animation cap rose from three frames to four: `tileset::kMaxFrames` drives everything, so the atlas and layer PNGs widened from 24 to 32 pixels, the generator regenerated the committed tilesets, the frame row gained a fourth button with keys 1-4 selecting frames in the Tiles view, the Clr cascade and first-stroke-copies semantics extend to frame 4, and the frame-strip previews shrank to 24x24 to keep the anim box left of the library grid.
Shift+Tab cycles the views in reverse; key events already carried the shift modifier, so no backend change was needed.
Hotkeys are now rebindable: twenty letter/function actions (heights, bridge, light, undo/redo, save/reload, generate, validator, entity fast paths, stamps, draw-color toggle, playtest, fullscreen) resolve through a bindings table with an Edit > Keys... modal — click a row, press the new key, conflicts and reserved keys (digits, Tab, Escape, grave, arrows) are refused with a message, and [Defaults] restores the shipped set — persisted as a "keys" object in the config file beside uiScale and fullscreen.
View-dependent behavior stays attached to the action, so a rebound save key still saves the map or the tileset per view, and F5/F10 no longer fire while a text field is focused since they resolve through the same table.
Verified under Xvfb with synthetic input: New produced the uniform floor field with zero free cells and a reset camera; key 4 plus one stroke created frames 2-4 as copies of frame 1 and a single undo removed them; Shift+Tab stepped Map to Characters; raise-height rebound from E to Y, took effect immediately, was refused when a second action tried to claim Y, and survived an editor restart via config.json; the regenerated 32-wide assets passed `--check` and rendered the demo map with borders, decor, and water animation intact; full suite 6812 tests green and all four style checkers clean.

## 30: generated-combination preview panel in the tileset workspace

The Tiles view gained an always-visible preview panel: an 11x5 lattice of 8x8 sprites at game scale below the frame strip, showing the selected sprite pinned at an outlined center cell inside a valid generated combination of its tileset.
The region shape is carved from the selected sprite's `edge` sides (everything beyond an `edge` side of the center is outside), the base fills in rings outward from the pinned center under the assembler's shape-fit and socket-match rules with the same progressive relaxation ladder, and every decor layer then scatters on top per allowlists, decor sockets, and density — so the panel composites all layers and animated sprites cycle their frames on the global clock.
Selecting a decor sprite pins it at center over a seeded pick from its sits-on allowlist; an empty allowlist shows the base-only neighborhood with a "no base sprites allowed yet" message.
A custom-drawn regen button rerolls the seeded arrangement and an auto checkbox increments the seed every 90 ticks for hands-free cycling; both are view state, outside undo and outside the config file, and the generated result is cached by tileset revision, selection, and seed following the map plan-cache precedent.
Generation lives in the app's TilesetPreview.cpp with an xorshift32 stream, deterministic per seed.
Verified under Xvfb: a grass base sprite previewed with flower and pebble decor on allowlisted bases only; an N-edge wall sprite previewed with the rows above it outside and the border running through the center; a flower pinned over grass_a; a freshly added decor sprite with no allowlist produced the message; a regen click changed 2106 preview pixels; auto-mode captures two seconds apart differed without input and were pixel-identical once unchecked; full suite 6732+80 green and all four style checkers clean.

## 31: per-level slab columns — overhangs, tunnels, schema v4

The one-cell-one-height model is gone: a cell now holds a column of one-level-thick solid slabs (level, terrain, overlay, water, light), so a floor at level 0 can sit under a roof at level 3 — the artist draws tunnels and overhangs per height level.
The player is two levels tall (kClearance = 2 in the tilemap lib), so a surface is standable only with two empty levels above it, and a walkable tunnel needs its roof at least three levels above its floor.
Movement, shared by validator and demo, follows the landing rule: the reachable neighbour surface is the topmost slab at or below your level (equal walks, lower is a one-way drop — you land on roofs, never through them), plus a surface exactly one level up via a stair on either end; water currents now bypass only the stair requirement rather than arbitrary rises.
Rendering keeps every public signature: a slab contributes sprites only when exposed (no slab directly above, preserving pixel parity with migrated maps), faces are per-slab bands where the south column lacks that level (off-map south counts as solid up to level zero), shades and bridges iterate slabs, and the cutaway's rises test plus a new own-column seed opens roofs above the player or the hovered cell.
Map schema v4 replaces the dense terrain/height/light and sparse bridge/water sections with a sorted per-level "levels" array of row strings (dot = no slab), entities carry a level, and schemas 1-3 load by solid-filling each cell from the map's lowest level up to its height with attributes on the top slab — capped at a 64-level span so pathological legacy files fail cleanly instead of exhausting memory.
The editor paints on an active level stepped with E and Q (the raise and lower height commands died, hotkey config tokens kept), right-click erases the active level's slab and pins the authored hole, bridges and light act on the active level's slab, stamps copy whole columns, entities place at the active level and draw markers on their own level, Generate fills free cells at the active level while pinned holes drop their adjacency constraints, and hovering cuts away everything above the active level so tunnel interiors stay editable.
mapcheck was rewritten over (cell, level) surfaces and gained its first test suite — seventeen tests at full branch coverage — with findings carrying an optional level, per-column CellReach for the overlay, and a distinct finding when an entity rests on no standable surface; the tilemap suite grew from fifty-three to ninety tests across the Slab/Column model, the v4 codec, and the migration paths.
Verified under Xvfb end to end: a wall massif pierced by a ground-level tunnel authored entirely with synthetic input, the roof auto-cutting while hovering the bore, a schema-4 save whose levels array round-trips pixel-identically through a reload, the demo player walking through the tunnel at level 0, climbing a stair chain to stand at level 3 directly over the bore, dropping off the roof edge onto the ground, a one-air-level bore correctly rejected by the validator, and the untouched schema-2 demo map rendering identically through migration; the full suite stands at 6786 plus 80 conformance tests, all green.
Authoring guidance from verification: wall tops are never walkable in either the demo or the validator, so a roof meant to be crossed must be painted floor, path, or stair.

## 32: artist-controlled per-sprite frequency weights

Every tileset sprite carries a weight from 1 to 16 (default 4), and wherever the assembler must choose among several equally valid sprites — same shape fit and matching sockets, or several eligible decor candidates — the deterministic hash now picks proportionally to weight instead of the old hardcoded rule that gave the lowest declared sprite half the buckets.
Weights bias only among already-valid candidates and never override socket or shape validity; the favored-sprite concept is gone, and the relaxation ladder's candidate picks and the decor scatter use the same weighted selection, mirrored in the workspace's preview panel.
The JSON codec writes the weight only when it differs from the default, so existing tileset files and the canonical-bytes contract are unchanged; values outside 1..16 are rejected.
The Tiles panel gained a weight stepper for the selected sprite under the sprite row, visible in every tool mode, undoable through the tileset snapshot stack and live in the map preview; library-cell hints append the weight when it is not default.
The placeholder floor tileset demonstrates the feature with pebbles at weight 2, rarer than flowers, and the generator emits and checks the member byte-identically to the C++ writer.
Verified by a statistical harness assembling a 40x40 lattice: two same-socket interiors at weights 15 and 1 land at a 6.3 percent rare share against the expected 6.25, equal weights land at 48.3 percent, decor ratios match, and two runs are byte-identical; under Xvfb the stepper stepped and clamped, undo reverted, the weight persisted through save and relaunch, and the preview re-rolled on the change; full suite green and all four style checkers clean.

## 33: sprite picker with layer-cycling picks in the map view

A picker (eyedropper) lets the artist click a sprite on the rendered map to select it in the tileset workspace, so finding the sprite behind any pixel takes one click instead of a hunt through the library.
The tool toggles via a Pick button in the map panel, a rebindable hotkey (default I, config token "picker"), or Escape to leave; ctrl+left-click is a one-shot pick without entering the mode, and painting, erasing, and entity selection are suppressed while the mode is on.
A pick inverts the camera to the clicked plan point, collects the 8x8 sprite draws covering it from the cached draw plan in bottom-to-top order, and walks that stack on successive clicks at the same lattice spot — base sprite first, then the decor layered on it, wrapping — with the walk resetting when a different spot is clicked.
The chosen draw's atlas row reverse-maps through the tileset's layer offsets to activate the bound tileset document and set its layer and sprite selection without leaving the map view; the tileset dropdown and the picker now share one activation path.
Terrains rendering from the compiled placeholder report "placeholder tileset - not editable" in the hover hint, the preview label, and the workspace message instead of selecting anything.
Feedback is a hover hint naming what the next click would pick and which layer follows, plus a preview box above the hint line showing the picked sprite's art at 2x with its tileset, layer, and index, visible while the mode is on or for three seconds after a pick.
The keys dialog grew to twenty-one rows by tightening its row pitch with a gap-free inner column.
Verified under Xvfb: hover hint on a decor-bearing floor cell, first click selecting the base grass sprite (focus ring confirmed in the Tiles view), second click selecting the flower decor with its layer row pressed, third click wrapping to base, ctrl+click with the mode off selecting a wall sprite, I and Escape toggling with painting restored after exit, the new action rebinding to M and back, and the empty-tilesets placeholder path showing the not-editable message; full suite 6871 green and all four style checkers clean.

## 34: playtest map flag, brush-independent erase, erase hint

Three defects surfaced by the user actually authoring maps, fixed as one batch.
F5 saved the map but launched the demo with no `--map` argument, so playtesting always showed the built-in hardcoded layout; the playtest command now appends the current map path, quoted, and the demo renders the freshly saved edits.
Right-click erase silently did nothing while the free brush was selected, an arbitrary coupling that read as "erasing is broken"; the brush check is gone, so the eraser works regardless of the selected brush and the erased cell is still pinned.
The hint line never mentioned the erase gesture, leaving it discoverable only in the guide; hovering a cell whose active level holds a slab now appends "right-click erases".
Verified under Xvfb: a wall patch painted on a fresh map appeared in the demo after F5 (process list shows the `--map` argument, the demo scene matches the edit instead of the built-in map), a right-click with the free brush selected removed a slab (panel and hint flip to "no slab"), and the hint text renders over slab-bearing cells; both apps rebuild warning-free and all four style checkers stay clean.

## 35: demo renders the map's bound tilesets with decor and animation

The playtest demo had rendered from compiled placeholder tilesets since the socket-tileset cutover, so F5 showed different art than the editor and no decor at all — acceptable while no real assets existed, misleading once the artist started authoring.
The demo now takes `--tilesets <dir>` (default `assets/tilesets`), loads the tileset library once at startup, loads the shared system sheet when it is the expected 32x8, and resolves each terrain exactly as the editor does: the map header's binding name, then `default-<terrain>`, then the compiled placeholder.
Bindings re-resolve whenever the header's binding names change (startup and console map reloads), atlases re-bake on palette changes through the existing path, and the draw loop is untouched — decor layers and per-sprite animation flow straight out of the assembler once the real sprites are bound.
A missing or empty tilesets directory falls back to placeholders without complaint, and nothing was deleted.
Verified under Xvfb: the demo on the committed demo map shows grass with scattered flower and pebble decor, default-wall art, and rippling water (timed frames differ exactly in a water region); an editor F5 launch renders identical art to the editor's map view with matching decor positions; `--tilesets /nonexistent` renders the old placeholders without crashing; full suite 6791 plus 80 conformance tests green and all four style checkers clean.

## 36: one-level character clearance

The character's logical height dropped from two levels to one: `kClearance` in the tilemap lib is now 1, so a surface is standable unless a slab sits at the level directly above it, and any gap with a single level of air — floor at L, roof at L+2 — is passable.
The validator and the demo already shared the constant, so the rule cannot diverge; the mapcheck suite reworked its low-roof, tunnel, stair-headroom, and buried-entity scenarios to pin the new boundary (roof at L+1 blocks, L+2 passes), and the guide's overhang, headroom, and stair sentences follow.
Verified under Xvfb: the demo player walked under an L+2 roof (pos level 0 inside) and was blocked by an L+1 roof; tilemap 90/90, mapcheck 17/17, the asset gate, and all style checkers green.

## 37: icon toolbars with hint-line tool names

Tool controls became square 10x10 icon buttons: the six terrain brushes draw their bound tileset's first base sprite baked with the map palette (falling back through default and placeholder resolution automatically), the free brush a dashed-cell glyph, the picker an eyedropper, and the Tiles tabs a pencil, a socket plug, and a flower; the active tool inverts.
Hovering any icon shows the full tool name with its key in the hint line, and the brush label above the row keeps a persistent readout.
The mechanism is app-side icon overdraw on unchanged ui-lib buttons — the shared ui lib's draw commands, coverage gate, and every other app stay untouched, and dispatch, keys 1-7, hover, and the 2x/3x/4x scales all kept working (verified by screenshot at each scale).
Adding a tool later costs one button call, one glyph, and one hint.

## 38: select tool — marquee, move, cut, copy, paste in all views

A Select tool (marching-ants icon) joined every toolbar, including a first two-icon toolbar for the Characters view.
With it active, a left drag marquees a region — cell-aligned columns on the map, pixel rects in the tileset sprite editor and the character sheet — and ctrl+C, ctrl+X, and ctrl+V copy, cut, and paste it; dragging from inside a selection moves it as one undo step, cut and vacated regions become empty (map cells stay pinned so Generate leaves the holes), and paste anchors at the hovered cell or pixel and works from any tool.
The map clipboard carries whole columns level-absolutely like stamps; the tile and character editors share one pixel clipboard whose blank pixels paste transparently, so art moves freely between a character sheet and a sprite.
Escape clears the selection first and then leaves the tool; undo and redo clear selections; chords respect field focus and never collide with the plain C draw-color toggle; the raylib key events already carried the control modifier.
Verified under Xvfb across seventeen screenshots: map copy, cut leaving pinned holes, clipboard surviving a cut, a (2,2) move restored by a single undo, the Escape chain, tile-to-tile paste with transparent blanks, character head cut and re-paste, a cross-view character-to-sprite paste, and plain C still toggling the draw color; full suite 6791 plus 80 green and all four style checkers clean.

## After the queue drains

Open threads deliberately not in the queue: editor unit tests and replay coverage (blocked on the deferred-tests decision and the WindowedSession migration), boat traversal edges in mapcheck, hover mapping ignoring elevation lift, and the void left where cutaway hides a block.
