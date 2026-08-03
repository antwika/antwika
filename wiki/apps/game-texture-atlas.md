# The game's texture atlases: what an artist has to produce

`apps/game` blits everything it draws from three hand-drawn sheets under `src/apps/game/assets/`, one per footprint size:

```
atlas_1x1.png   512 x 768    64 x 96 sprites    ground, roads, walkers, 1x1 buildings
atlas_2x2.png   768 x 896    96 x 112 sprites   2x2 buildings
atlas_3x3.png   1024 x 1024  128 x 128 sprites  3x3 buildings
```

The art is the source of truth for how the game looks; nothing generates or rebuilds it, and editing a sheet is editing the art.
The one predecessor sheet *was* generated, and [`blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md`](../../blog/019-the-generated-atlas-was-the-wrong-kind-of-correct.md) says why that stopped.

`src/apps/game/include/antwika/game/TileAtlas.hpp` is the address map.
It is the one place that says where a sprite lives and what shape a sheet is, and it says so arithmetically rather than as a table of rectangles, so there is no list in the header that could disagree with the pictures.
**Repainting a sprite is therefore free.**
**Moving one is not**, because the header is what decides which pixels an index means.

This document is the contract between that header and whoever draws the art.
Everything in it is a way for a correct-looking sprite to be wrong once the game blits it.

## The sheets

Each sheet is a PNG of **8 columns by 8 rows** of sprites, and the three differ only in how big one sprite is.
Sprite *n* has its top-left corner at `((n % 8) * width, (n / 8) * height)`, counting left to right then top to bottom from sprite 0.

Any PNG the decoder accepts will do — indexed, greyscale, 16-bit and interlaced all decode to the same 8-bit RGBA — so export whatever your tool is happiest with.
The size, however, is checked per sheet: an export that is not exactly the size above is refused at startup with a message naming the file and both sizes.
That check exists because `gfx::blitIsDrawable()` refuses an out-of-range source rectangle **silently**, so a sheet exported one row short would draw a blank grid without failing, logging or crashing.

## One sprite's geometry, and the pivot

All three sheets draw to the same isometric grid: **one cell's diamond is 32 by 16 pixels** of art.
A sprite is bigger than its diamond, and the same shape at every size:

- **The pivot** is the bottom corner of the sprite's footprint diamond, at `(width / 2, height - 32)`: **(32, 64)**, **(48, 80)** and **(64, 96)** respectively.
- The footprint diamond — 32x16, 64x32 or 96x48 — sits immediately above the pivot, so its top corner is at **y = 48** in every sheet.
- **48 pixels of headroom** above the diamond's top corner, for walls, roofs and anything else that stands up.
- **16 pixels of margin** either side of the diamond.
- **32 pixels below the pivot**, for the base block's skirt (see below) and its padding.

Blitting anchors the pivot to the block's own bottom corner on screen (`SpriteBounds.hpp`), so headroom rises above the cell and the skirt hangs below it — a sprite lands on its cell however much of its box the art uses.
Every one of those numbers is a multiple of 16, and must stay one: the furthest zoom scales art by a quarter, and `SpriteBounds.hpp` refuses at compile time any sprite metric that would round there.

## The base block and its skirt

A ground or road sprite is drawn as a shallow **block** rather than a flat diamond: its top face is the footprint diamond, and **16 pixels of side faces** hang below the pivot as a skirt.
Buildings carry the same skirt under their own block.

The skirt is covered by whatever stands one cell south or east — the scene paints terrain and buildings back to front, a diagonal of cells at a time, precisely so that it is — and it shows only along the map's south and east edges, where it reads as the ground's own cliff.
What that buys is that nothing has to special-case an edge, and what it costs is the paint-order rule the scene now owns; see `GridScene`.

**A walker sprite carries no base block**: only the figure, standing so its feet rest mid-diamond, a few pixels above the pivot.
The cell it stands on supplies the ground.

## The 1x1 sheet's slot table

This is the contract, in this order.

```
sprite  count  what
0       1      grass, the default terrain
1-16    16     roads, ordered by which arms they join (below)
17-21   5      the 1x1 buildings: house, well, doctor,
                 fire station, engineer post
24-27   4      walker facing NE on screen (grid north), row 3
32-35   4      walker facing SE on screen (grid east), row 4
40-43   4      walker facing SW on screen (grid south), row 5
48-51   4      walker facing NW on screen (grid west), row 6
```

Every other sprite is currently unused: the game never blits one, so what an unused cell holds is free — today most hold the template block the sheet was ruled out from.
Each walker row's four sprites are that facing's walk cycle, cycled left to right once per cell crossed; the first doubles as the standing frame, which is why an idle walker needs no fifth sprite.
The rest of each walker's row (28-31, 36-39, 44-47, 52-55) stays reserved for more frames.

## The roads, and the mask-to-sprite table

A road's arms are named **on screen** in the sheet — NE, SE, SW, NW — while the game names its neighbours in **grid** space, and the projection shears one onto the other the same fixed way everywhere: grid north shows as the NE arm on screen, east as SE, south as SW and west as NW.

The sheet orders its junctions by arm count rather than by link mask, so `TileAtlas.hpp` carries the crossing as the sixteen-entry `kRoadSpriteByLinks` table — the one place the two orders meet, pinned by `TileAtlasTest`:

```
sprite  arms on screen        sprite  arms on screen
1       none                  9       NW+NE
2       NE                    10      NE+SE+SW
3       SE                    11      SE+SW+NW
4       SW                    12      SW+NW+NE
5       NW                    13      NW+NE+SE
6       NE+SE                 14      SE+NW   (straight)
7       SE+SW                 15      NE+SW   (straight)
8       SW+NW                 16      all four
```

**A road sprite must be opaque across its whole diamond**, since it is blitted over a grass sprite rather than instead of one, and its stubs must run along the diamond's own axes or a junction will not meet the road it joins.

## The walkers

One facing per row, rows 3 to 6, in `Direction` order — and **facing is in grid space, not screen space**: north is up-and-*right* on screen, east is down-and-right, south is down-and-left, west is up-and-left.

**Each row's first four sprites are that facing's walk cycle**, in walking order left to right, and the first of them doubles as the standing frame.
Which frame shows is `WalkerMotion.hpp`'s `walkerFrame()`: one whole cycle per cell crossed, resolved from the same exact step fraction that slides the walker between cells, so the legs cannot drift against the ground and a paused run freezes both together.
An idle walker — one that has not yet taken a step — holds the standing frame.

**A walker sprite is meant to be tinted at blit time, once per walker kind.**
There are seven kinds — water carrier, doctor, fireman, engineer, cart pusher, market buyer and market seller — and four facings, and the art is four sprites rather than twenty-eight because the kind is meant to be applied as a colour multiply over the facing's sprite.
So **draw the walkers in a neutral, fairly light value**: a sprite that is already strongly coloured cannot be tinted convincingly, and one that is already dark goes black.
`GridScene` does not apply that multiply yet and blits every walker untinted, so telling one kind from another on screen is a standing debt rather than a thing the art can fix.

## The buildings

A building's sheet is decided by its footprint — `buildingAtlasOf()` derives it, so a 2x2 kind cannot end up drawn from a diamond its block does not fit — and its sprite index by `kBuildingSprites`:

```
1x1 sheet          2x2 sheet        3x3 sheet
17  house          0  farm          0  storehouse
18  well           1  clay pit
19  doctor         2  workshop
20  fire station   3  market
21  engineer post
```

**A building's art owns its whole footprint**: the scene lays no grass under a standing building, so the sprite must cover its whole footprint diamond (skirt included), or the sky shows through.
Outside the diamond, the headroom and margins are the building's to use and must stay transparent where unused.

**Tell one building from another by silhouette rather than by hue.**
Colour is already spoken for: the walkers' hue carries their kind, and a building competing with that makes the two read as one language.

**A building must also read at the placement ghost's alpha.**
The preview is the same sprite drawn at alpha 110 out of 255, and when a block will not fit it is drawn at that alpha *and* tinted red (255, 90, 90).
A silhouette that only works opaque will not survive either.

## Five zoom levels, and where the art is one-to-one

The camera has five zoom levels, and one cell's diamond is drawn at **8x4, 16x8, 32x16, 64x32 and 128x64** pixels — so the art is one-to-one at the middle level, scaled down to a half and a quarter at the far two, and scaled up twice and four times at the near two.

Two consequences to design around.
**Detail below roughly four art pixels disappears at the smallest zoom**, so a rim specified in pixels rather than as a fraction of the diamond is the usual casualty.
**The near zooms magnify pixels rather than adding any**, so the closest view is deliberately chunky; art that only reads when its pixels are square-on-screen is art that only works at one level.

## The ground and the grid lattice

**The grid lattice is painted into the grass sprite's own edges.**
`GridScene` draws no shape of its own bar the placement ghost's border, so a grass sprite that loses its rim loses the grid with it.

Two sprites drawn to the same diamond tessellate exactly, which is the whole reason to respect it: a pixel at `(px, py)` of a sprite is inside the footprint diamond when, with `cx` the pivot's x and `cy = 48 + footprint height / 2` the diamond's centre line,

```
dx    = px + 0.5 - cx
dy    = py + 0.5 - cy
east  = (dx / w + dy / h) / 2
south = (dy / h - dx / w) / 2
```

satisfies `|east| <= 0.5` and `|south| <= 0.5`, where `w` and `h` are half the footprint's width and height.

## The art debt these sheets are carrying

**Most of the building sprites are placeholder blocks and want drawing properly.**
A placeholder is the template block — the footprint diamond and its skirt in a flat fill — which respects every rule above and says nothing about what the building is.
The walkers are small neutral figures, four walk-cycle frames per facing.

## What is left checking the art

Almost nothing, and that is the deliberate trade for hand-drawing it.

- The `static_assert`s in `TileAtlas.hpp` catch a slot table that does not fit a sheet, a road table that is not a permutation, and two kinds sharing a sprite.
- The ones in `SpriteBounds.hpp` catch a sprite metric that would not scale exactly at every zoom.
- `TileAtlasTest` pins the sprite arithmetic, the road table, the ranges and their non-overlap; `SpriteBoundsTest` pins where a sprite lands.
- `game::requireAtlasSize()` refuses a sheet that is not its expected size, at startup, naming the file and both sizes.

None of those looks at a single pixel.
Whether a diamond is the right diamond, whether a road stub meets its neighbour, and whether a walker faces the way the game says it does are all things only a person looking at the running game will catch.

`build/bin/antwika_game/antwika_game --replay src/apps/game/replays/demo.jsonl` under an `sdl3` build is the quickest way to look at one.
