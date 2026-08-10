# Map system requirements

This document records the requirements for the game's tile, map, and map-tooling system, as decided on 2026-08-10.
It is a statement of intent for work that has not started; nothing in this document describes existing code.

## Product shape

The game's working title is Wakewater, and code, directories, and targets use the slug `wakewater`.
The display title stays out of user-facing strings except through one central definition, so a later rename stays a directory move rather than a hunt.
The game is a top-down action-adventure structured around Metroidvania-style floor plans, where branching paths, verticality, and ability-gated progression are core design goals.
The world is a graph of discrete maps connected by door and stair transitions, with a cut or fade between maps rather than seamless scrolling.
The world consists of fewer than twenty maps, each roughly 50×50 to 100×100 units.
Maps are authored and generated at design time in an in-repo editor, reviewed by a human, and shipped fixed.
Nothing is generated at runtime.
All map code lives in this repository and is written in C++ against the existing raylib backend.

## Presentation

The game renders at a virtual resolution of 320×180 and scales only by integer factors, so 1-bit pixels stay crisp at every window size.
At 16×16 pixels per unit that shows roughly 20×11 units per screen.

## Art format

All map art is 1-bit pixel art in a grimey-mechanical style.
Art is authored against a two-color palette, and each map's header names its own ink and paper colors, giving near-free per-area mood variation.
Sprites are 8×8 pixels, half the width of a unit, and compose 2×2 into one 16×16 rendered unit.
Tiles follow the dual-grid corner scheme: each rendered sprite is selected by the terrain values at its four surrounding corners, so tile selection is a deterministic lookup and never a generation problem.
Terrains render in layers over transparency in a fixed draw order, so each terrain needs transitions only against transparency rather than against every other terrain.
The projection is 3/4 top-down in the JRPG style, which breaks vertical symmetry: north and south edges of a terrain require distinct art, while horizontal flips remain valid.
Grime and visual variety come from a small set of alternate sprites for common tiles, selected deterministically by position hash, rather than from additional terrain types.
Some sprites are animated as short frame loops (water ripples, currents, machinery), all driven by one global tile clock.
Sprites are delivered as PNG spritesheets committed to the repository, laid out in a fixed grid convention where sheet position encodes the corner mask.
A small JSON sidecar per sheet lists only what the convention cannot encode: variant counts and animation frame counts.
The build depends on no external art tool.

## Semantic map model

A map is a grid of cells plus a list of entities.
Terrain classes are a fixed C++ enum, so adding one is a code change with exhaustive-switch safety.
Each cell holds:

- An integer height with no designed upper bound.
- A terrain class (floor, wall, water, cliff, path, stair).
- An optional overlay, initially only bridge.
- Optional water attributes (deadly, swimmable, current direction), reserved in the schema from day one even though most are implemented later.
- A painted light level, rendered as a static dither-mask darkness overlay with no runtime light simulation.

Doors and boat embark points are entities in the entity list, not tiles, because they carry gameplay state and behavior.
A door or stair transition entity names its target map and entry point.
The map system also owns enemy spawn points, item pickups and chests, NPC markers whose IDs game code binds behavior to, and trigger volumes.
Trigger volumes are rectangular region entities that fire named events; every other entity kind is a point.
Abilities and keys that gate progression are free-form string tags in map data: a gate requires a tag, a pickup or event grants one, and tooling treats tags opaquely.

## Serialization

Maps are stored as JSON, read and written with the repository's existing nlohmann::json dependency, and committed to the repository.
Grid data is encoded row by row so diffs stay readable and reviewable.
Each file carries a schema version so later phases can migrate maps mechanically.
A CI gate runs the validator over every committed map, so a broken map fails the build like a failing test.

## Elevation

Height is an unbounded integer per cell.
Cliffs are the rendered boundary between cells of different height.
Because height is unbounded, cliff faces cannot be single sprites: each face is a rim sprite plus a vertically tiling 8×8 wall-band sprite repeated once per level of drop.
Stairs connect adjacent height levels and are the only way to gain height.
Cliff edges are one-way: an actor can drop from a higher cell to a lower one but never climb back up.
Verticality is a primary level-design tool, so one-way drops are expected to form shortcuts and commitment points, not just scenery.
Actors may walk behind tall terrain: narrow, deep pathways hidden behind high structures are an intended level-design tool, and no design rule forbids occluded walkable space.
When terrain occludes the player, the renderer hides the entire occluding block rather than fading individual tiles: every cell of the connected high region is omitted from the player's height level upward, revealing the space behind it.
This requires the renderer to draw terrain in per-height-level passes and to compute connected regions of high ground.

## Water

Water blocks movement by default.
Traversal mechanisms are phased in this order:

1. Bridges, which are pure tile data over water and require no new movement code.
2. Swimming, which makes flagged water cells walkable under an ability gate and requires art distinct from deadly water.
3. Boats and rafts, which allow traversal only between designated embark entities.
4. Currents, which add flow direction to water and act as one-way edges like cliff drops.

The cell schema carries the fields for all four phases from the start, so early maps never need migration.

## Movement and validation

Actors move freely in pixels over per-tile collision derived from the semantic grid.
Collision resolves per 16×16 unit cell, exactly matching the validator's cell graph, with small bevels on convex collision corners so free movement does not snag.
Bevels affect movement feel only and never change reachability.
Vertical traversal consists of stairs upward and one-way cliff drops downward.
Because all traversal is determined by the grid, reachability is an exact graph problem.
The validator answers, for a given ability set, which cells and entities are reachable from a given entry point.
The graph includes one-way edges for cliff drops and, later, currents.
The validator must flag unreachable required entities, dead-end regions with no return path, and gates reachable before their intended abilities.
Required progression includes placed items, so a key sitting in an unreachable chest fails the CI gate.
Because ability tags are free-form strings, the validator cross-checks them: every tag a reachable gate requires must be granted somewhere reachable first, so a typo surfaces as a validation failure rather than as a gate that silently never opens.
A second, world-level pass validates the map graph itself: every transition must name an existing counterpart that leads back consistently, and progression must be reachable across maps, not just within them.
Validation runs inside the editor so problems surface while the map is being authored.

## Tooling

The artist authors and reviews maps in a raylib-based editor application in this repository.
Beyond the basics of painting and saving, the editor commits to:

- Brush painting of terrain and a height brush with per-level isolation views.
- Undo and redo, built on a command history from the first commit because retrofitting it is painful.
- Region select with copy and paste, including regions saved as reusable prefab stamps.
- Placement and property editing of entities (doors, boat points, keys, gates) without hand-editing JSON.
- A live autotiled preview and the validator's results as an overlay.
- A playtest handoff that spawns the player into the running game at the cursor position.

The editor is a thin shell: autotiling, the map model, and validation are pure libraries with tests, matching the repository's coverage gates.

## Role of generation

Macro layout is game design, and the designer owns it.
With fewer than twenty hand-reviewed maps there is no volume to amortize a structural generator against, so room-graph, BSP, and wave-function-collapse generation of layout are out of scope unless painting proves tedious in practice.
Prefab stamps are the manual alternative: recurring structures are drawn once and stamped, not generated.
Automation is invested where it saves artist time per tile rather than per map:

- The deterministic dual-grid autotiler, which reduces each terrain to roughly six to ten hand-drawn sprites.
- Position-hashed variant scattering for grime, which needs no authoring per map.
- The reachability validator, which replaces eyeballing as maps interconnect.
- Optional micro-detailing passes, such as cliff-edge noise or water pools, added only if hand-painting them proves tedious.

## Milestones

1. Autotiler: a pure, tested library plus a small raylib app that renders a hardcoded semantic grid with placeholder 1-bit art, proving 3/4 elevation rendering, wall-band tiling, draw order, cutaway occlusion, and the 320×180 integer-scaled presentation.
2. Editor shell: paint cells, height brush, save and load as versioned JSON, live autotile preview, with undo/redo in the architecture from the start.
3. Validator: the exact reachability graph with one-way edges and ability sets, surfaced as an editor overlay and enforced as a CI gate over committed maps.
4. Entities and the world graph: entity placement and editing, door transitions between maps, and the world-level validation pass.
5. Variants, grime scattering, and animated tiles.
6. Region stamps and the playtest handoff.
7. Bridges, then the remaining water phases in order.
8. The painted lighting overlay.
9. Generation experiments, only if authoring friction justifies them.
