# Tileset convention

This document describes the on-disk tileset format every terrain's art follows.
The `tileset` library is the authoritative encoding of this convention, and `scripts/generate_placeholder_tiles.py` renders placeholder tilesets that follow it.

## Directory layout

A tileset is one directory under `assets/tilesets/`, named after the tileset.

```
assets/tilesets/
  system.png
  rules.json
  default-wall/
    tileset.json
    layer-0.png
  default-floor/
    tileset.json
    layer-0.png
    layer-1.png
```

`tileset.json` describes the tileset and its sprites, and each `layer-<i>.png` carries the pixels of layer `i`.
A layer that declares no sprites has no layer image.

## The tileset document

`tileset.json` holds a single object with a fixed member order.

```json
{
  "schema": 1,
  "name": "default-wall",
  "terrain": "wall",
  "nextSpriteId": 3,
  "layers": [
    {
      "name": "base",
      "sprites": [
        {
          "id": 0,
          "sockets": {
            "n": "edge",
            "e": "wall",
            "s": "wall",
            "w": "edge"
          },
          "on": []
        },
        {
          "id": 1,
          "sockets": {
            "n": "wall",
            "e": "wall",
            "s": "wall",
            "w": "wall"
          },
          "on": []
        }
      ]
    },
    {
      "name": "moss",
      "density": 48,
      "sprites": [
        {
          "id": 2,
          "frames": 2,
          "sockets": {
            "n": "open",
            "e": "open",
            "s": "open",
            "w": "open"
          },
          "on": [
            1
          ]
        }
      ]
    }
  ]
}
```

`schema` is 1, `name` names the tileset, and `terrain` is one of `floor`, `wall`, `water`, `cliff`, `path` and `stair`.
Sprite ids are unique across every layer and `nextSpriteId` clears the highest id, so an editor can allocate new ids without rescanning.
`frames` is omitted when a sprite has one frame, `weight` at its default of 4, `density` on the base layer and at its default of 64, and `sockets` and `on` are always present.
The document is written with two-space indentation and a trailing newline, and equal tilesets serialize to equal bytes.

## Layers

Layer 0 is the base layer, and its sprites tile the terrain region completely.
Every layer at index 1 and above is a decor layer, whose sprites scatter over the base.
A decor layer's `density` is the scatter threshold from 0 to 255, so higher values place more decor.
A decor sprite's `on` array lists the base sprite ids it may sit on, so a flower can demand grass and refuse dirt.
The `on` array stays empty on base sprites.

## Sockets

Each sprite names a socket per side, under `n`, `e`, `s` and `w`.
Two sprites may sit side by side exactly when the sockets facing each other carry the same name.
Socket names are free-form per tileset, except for two reserved names.
The name `edge` marks a base-sprite side that faces outside the terrain region, so borders emerge from sockets alone: a sprite with `edge` on its north side is a top border piece, and one with `edge` on two adjacent sides is a corner.
The name `open` is what an empty cell presents on decor layers, so decor sprites that sit alone carry `open` on every side.
A bounded region renders from one interior sprite, four edge sprites and four corner sprites, which is the shape every `default-*` tileset ships.

## Sprite weights

A sprite may carry a `weight` from 1 to 16, defaulting to 4.
Wherever assembly finds several valid sprites for an 8x8 cell — interchangeable base variants or competing decor — each candidate is picked with probability proportional to its weight, so a weight-8 sprite appears about twice as often as a weight-4 peer.
Weights only bias the choice among valid candidates; they never override socket matching or shape fit.
The placeholder floor's pebble decor carries weight 2, so it turns up rarer than the flower.

## Layer images

`layer-<i>.png` is 32 pixels wide: four 8-pixel frame slots, with frame `f` of a sprite at `x = f * 8`.
The image is 8 pixels tall per sprite, and sprite rows follow the JSON order of the layer's `sprites` array.
A sprite may declare up to four animation `frames`, and the slots past its frame count stay fully transparent.
The placeholder water interior demonstrates animation with three ripple phases.

## Pixel classes

Art carries two drawable classes plus transparency: an opaque white pixel (255, 255, 255) is an ink-class pixel, an opaque mid-gray pixel (128, 128, 128) is a paper-class pixel, and zero alpha is transparent.
The actual colors always come from the map palette — the renderer bakes textures with the palette's ink and paper applied to their classes, so every map recolors the same art.
Load-time normalization classes any opaque pixel by luminance: 192 or higher becomes ink, and the rest becomes paper.
Base sprites are fully opaque, and decor sprites are mostly transparent with an ink motif.

## The system sheet

`assets/tilesets/system.png` is a single shared 32x8 sheet of four 8x8 pieces, left to right.

| Piece | Position | Content | Drawn |
| --- | --- | --- | --- |
| Wall band | 0,0 | The vertically tiling cliff-face band. | No |
| Wall rim | 8,0 | The cliff-face rim under a surface edge. | No |
| Bridge deck | 16,0 | The bridge planking drawn over a cell. | No |
| Shade | 24,0 | The dither tile the lighting pass draws. | Yes |

Only the shade slot reaches the screen.
The renderer tints Shade draws black, so that slot's ink only defines the dither shape.

## Missing art

Every visible surface must be art an artist drew, and the renderer fills solid red (255, 0, 0) wherever that is not yet true.

Three kinds of surface are red always, because no tileset can supply them: the cliff-face wall band, the cliff-face wall rim and the bridge deck.
The system sheet still carries a piece for each, but nothing draws them, and the red says so rather than passing programmer art off as finished.

A tile sprite is red when its row lies past the baked atlas, when the bound tileset holds no sprite at that row, or when the frame being drawn is entirely blank.
A blank frame bakes to fully transparent, so without the marker it would leave an invisible hole instead of an obvious gap.

The `default-*` tilesets are drawn art and never come out red.

## Generation rules

`assets/tilesets/rules.json` makes the generation weights and the terrain adjacency artist-editable.
The document holds a `weights` object with a positive number per generatable terrain (floor, wall, water, cliff, path — stair is artist-only and keeps its fixed weight) and an `adjacency` array of two-name pairs; each listed pair is applied symmetrically, self-pairs included, and unlisted pairs are forbidden.
Unknown terrain names, non-positive weights, or malformed pairs make the file invalid; a missing or corrupt file logs a warning and the compiled-in defaults apply instead.

## Regeneration

Run `scripts/generate_placeholder_tiles.py` to rewrite `assets/tilesets/`: the six `default-<terrain>` tilesets, the system sheet and a defaults-matching rules.json.
Run it with `--check` to report whether the committed files match the generator, which CI enforces.
