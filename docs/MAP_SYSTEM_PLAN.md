# Map system implementation plan

This document is the step-by-step implementation plan for [MAP_SYSTEM_REQUIREMENTS.md](MAP_SYSTEM_REQUIREMENTS.md), written 2026-08-10.
Each step is a separately landable unit of work that passes the repository's coverage, style, and test gates on its own.
Steps are ordered by dependency; later steps assume everything before them has landed.

## Ground rules

New code follows the existing library and app conventions: `antwika_add_library` under `src/libs/`, `antwika_add_app` under `src/apps/`, tests beside the code, and 100% line, function, and branch coverage.
The map system splits into three new pure libraries and two apps, keeping the editor a thin shell as the requirements demand:

| Unit | Kind | Purpose |
| --- | --- | --- |
| `tilemap` | library | Semantic map model and JSON map files. |
| `autotile` | library | Dual-grid sprite selection, wall bands, cutaway, draw-plan generation. |
| `mapcheck` | library | Reachability validation, tag cross-checks, world-graph pass. |
| `map_editor` | app | The authoring tool. |
| `tilemap_demo` | app | Milestone 1 proof app, later a rendering scratchpad. |
| `wakewater` | app | The game itself (working title Wakewater), created outside this plan. |

Existing code is reused rather than duplicated:

| Existing unit | Role in the map system |
| --- | --- |
| `antwika::geometry` | `Grid`, `Point`, `Rect`, and `Size` underpin the cell grid and regions. |
| `antwika::gfx` | `ViewportRenderer` provides the 320×180 virtual canvas with integer scaling. |
| `antwika::atlas` | Sheet metadata handling; the tile-sheet sidecar follows `AtlasMetaFile`'s pattern. |
| `antwika::app` | `WindowedHost`, `WindowedSession`, and frame pacing for both apps. |
| `antwika::input` | Editor and demo input handling. |
| `antwika::config` | The nlohmann-based JSON document plumbing for map files. |
| `antwika::replay` | Record-and-replay tests for the editor loop, as the `game` app already does. |
| `antwika::wfc` | The existing constraint solver, only if step 13 ever happens. |

## Step 0: preconditions

Land the in-flight raylib-only renderer branch before starting, so the map system builds against a stable `IRenderer`.

## Step 1: `tilemap` — the semantic model

Create the `tilemap` library with the core value types and no I/O.
It holds the fixed `TerrainClass` enum, the `Cell` record (height, terrain, optional bridge overlay, reserved water attributes, light level), and a `TileMap` that wraps a `geometry`-based grid plus the entity list and a header.
Entities are the point kinds (transition, boat embark, spawn, pickup, NPC) and the rectangular trigger volume, all carrying free-form string tags where they grant or require abilities.
The header holds the map id, the ink and paper colors, and the schema version.
Tests pin the invariants: height is unbounded, water fields exist from day one, and every enum member is handled exhaustively.

## Step 2: map files

Add JSON serialization to `tilemap`, following the `AtlasMetaFile` split: a pure codec plus a thin file wrapper.
Grid data encodes row by row for readable diffs, files carry the schema version, and errors surface through a `MapError` type in the style of `AtlasError`.
Tests cover round-tripping, every malformed-input branch, and rejection of unknown schema versions.

## Step 3: `autotile` — masks, bands, cutaway, draw plans

Create the `autotile` library: given a `TileMap`, a camera rectangle, the player's cell and height, and a global animation clock, it returns an ordered draw plan of sprite references (sheet, sheet cell, screen position).
This is where the dual-grid corner lookup lives: each 8×8 half-tile's sprite is selected from the terrain values at its four surrounding corners, layered per terrain in fixed draw order.
Cliff faces expand into a rim sprite plus one wall-band sprite per level of drop.
Cutaway occlusion is computed here as pure logic: the connected region of high ground occluding the player is found, and its cells are omitted from the player's height level upward.
Because the output is a plain draw plan, all of this is golden-tested on tiny hand-written grids with no renderer involved.

## Step 4: sheet convention and placeholder art

Define the spritesheet layout convention where sheet position encodes the corner mask, and the JSON sidecar for variant and frame counts, parsed via the `atlas` library's patterns.
Add `scripts/generate_placeholder_tiles.py`, in the vein of `generate_poker_atlas.py`, emitting deliberately ugly but legible 1-bit placeholder sheets for every terrain.
Document the convention in this directory so the artist can replace placeholders sheet by sheet without code changes.

## Step 5: `tilemap_demo` — the milestone 1 proof

Create the demo app: a hardcoded map containing multi-level cliffs, stairs, water basins, and a tall structure with a narrow pathway behind it, rendered through `ViewportRenderer` at 320×180 with integer scaling.
A keyboard-driven player marker proves collision feel is out of scope here but cutaway is not: walking behind the tall structure must hide its connected block from the marker's level upward.
Exit criterion: the requirements' milestone 1 list (3/4 elevation, wall bands, draw order, cutaway, presentation) is visibly demonstrated, and a `PngWriter` screenshot lands in `docs/`.

## Step 6: `mapcheck` — the validator and its CI gate

Create the `mapcheck` library: it builds the traversal graph (walking, stairs upward, one-way cliff drops), answers per-ability-set reachability from an entry point, and reports the required diagnostics.
Diagnostics cover unreachable required entities, dead-end regions without return paths, gates reachable before their intended tags, and tag cross-checks where every required tag must be granted somewhere reachable first.
A world-level pass loads several map files and validates transition counterparts and cross-map progression.
Ship a small `mapcheck` CLI app and register a CTest entry that runs it over every committed map, making map validity a build gate exactly like a failing test.

## Step 7: `map_editor` — foundations

Create the editor app on `WindowedHost` with the smallest useful loop: terrain brush, height brush with per-level isolation views, save and load through `tilemap`, and the live `autotile` preview.
Undo and redo land in this step, not later: every mutation goes through a command history from the first commit, and the command types live in the app's object library where `antwika_add_app_tests` can cover them headlessly.
Editor sessions are covered with record-and-replay tests following the `game` app's `replays/` precedent.

## Step 8: entities and the world graph in the editor

Add entity placement and property editing for every entity kind, including transitions that name their target map and entry point.
Surface the `mapcheck` diagnostics as an in-editor overlay, including the world-level pass across the maps in the assets directory.
Exit criterion: a two-map world with a locked door and its key can be authored end to end without hand-editing JSON, and breaking either side of the transition fails validation visibly.

## Step 9: variants, grime, and animated tiles

Extend `autotile` with position-hashed variant selection (a deterministic hash of cell coordinates and a per-map seed, no runtime randomness) and frame-looped sprites driven by the global clock.
Extend the sidecar format and the placeholder generator to declare variant and frame counts.
Golden tests pin that the same map renders identically across runs.

## Step 10: stamps and the playtest handoff

Add region select, copy, and paste, with stamps persisted as map-fragment files reusing the step 2 schema.
Add the playtest handoff in its simplest honest form first: the editor launches the `wakewater` binary with a map path and spawn position on the command line.
This step depends on the `wakewater` app existing at all; if it does not yet, the handoff targets `tilemap_demo` as a stand-in.

## Step 11: water, phase by phase

Implement the four water phases in requirements order, each as its own landing: bridge overlays (model, autotile, traversal edges), swimming behind an ability tag, boat embark entities, and currents as one-way flow edges with animated art.
Each phase touches `tilemap`, `autotile`, and `mapcheck` together, and each extends the validator before the game learns the mechanic.

## Step 12: the lighting overlay

Add the light-level brush to the editor and the static dither darkness pass to the draw plan.
No traversal or validation change is involved unless the open question of light-gated progression is later answered with yes.

## Step 13: generation experiments (optional)

Only if authoring friction demands it, prototype semantic-layer pre-fill using the existing `antwika::wfc` solver with painted regions as pinned domains.
This step has no committed scope by design; the requirements place macro layout in the designer's hands.
