# The atlas is generated from the numbers that address it

*Post 17*

`apps/game` drew its isometric grid out of `drawLine` calls: a filled diamond per cell, a lattice made of the lines between them, a coloured lozenge for a walker.
[Post 13](013-the-camera-is-simulation-state.md) argued that `drawLine` had earned its place on the graphics seam because a lattice is lines.

It now draws every tile as one blit from one texture atlas, and the scene draws no shape of its own at all.
This post is about the texture path that made that possible, and about the checked-in PNG that is not really a source file.

## Textures, and the one thing they must not be able to do

`antwika::gfx` is a write-only projection: state goes out to the screen and nothing comes back, which is what lets a replay reproduce a run under the headless `NullBackend`.
A texture is the first thing in that library that looks like it could break the rule, because a texture is memory the application handed over and might want back.

`ITexture` therefore says nothing:

```cpp
class ITexture
{
public:
    virtual ~ITexture() = default;
    [[nodiscard]] virtual Size size() const = 0;
};
```

No pixel access, no framework handle, no render target, no screenshot.
Read-back is the one feature that would let rendering feed the simulation, so it is not in the interface at all rather than being discouraged in a comment.

Decoding is separate from uploading, and neither of them opens a file:

```cpp
[[nodiscard]] Bitmap read(std::istream &in) const;
```

`PngReader` takes a stream rather than a path, mirroring `ReplayReader` for the same reason: `antwika::gfx` opens no files, so every failure it can report is reachable from an in-memory stream and provable without a fixture on disk.
Every PNG colour type decodes to the same 8-bit straight RGBA — greyscale, palette and truecolour all gain an opaque alpha, 16-bit channels are reduced to 8 — so a caller never has to ask what was in the file and every backend uploads the same bytes.
Reading the file from disk is the application's job, which is why `antwika::app::readPngFile()` exists and `antwika::gfx` has nothing like it.

The decoder is stb_image, compiled in exactly one translation unit with `STB_IMAGE_STATIC`, which keeps every `stbi_` symbol internal to that file.
That is not tidiness — raylib links its own copy of stb_image, and as the file says, two sets of those symbols do not link.

## A texture belongs to the renderer that made it

This is the lifetime rule, and it is stated as a guarantee rather than as a warning:

> A texture belongs to the renderer that made it.
> Drawing it through any other renderer draws nothing, and destroying it after that renderer's window has closed is safe.

Both halves are load-bearing.
"Draws nothing" rather than "is undefined" means a mistake is a blank rectangle instead of a crash on one backend and a working picture on another.
"Safe to destroy afterwards" is what lets `main` declare a texture wherever it is convenient without a lifetime argument in the comments.

Making the second half true is the backend's problem, and both backends solve it the same way — each renderer knows its live textures, and frees them in `detach()`, before the framework tears the device down:

```cpp
// Before CloseWindow takes the GL context these need.
// Each is left valid but empty, since one may outlive us.
for (RaylibTexture *texture : liveTextures)
{
    UnloadTexture(texture->raw());
    texture->forgetRenderer();
}
```

`forgetRenderer()` is what turns "drawn after its window closed" into "draws nothing".
`main.cpp` still declares the texture after the window and therefore destroys it first, and says so in a comment — the guarantee exists so that getting the order wrong is not a crash, not so that the order stops mattering.

`createTexture` throws on failure while every drawing call stays silent, which is the same split `createWindow` already made: a caller that cannot have the resource it asked for has nothing to carry on with, whereas a caller whose blit was refused has a frame to finish.

## The scene stopped drawing shapes

With one blit per tile, `GridScene` no longer draws a single line.

The lattice is painted into the ground tile's own edges by the generator, which is a better picture than lines the scene places: the rim belongs to the tile, so it scales with the tile and cannot fall between two of them at an awkward zoom.

A junction is one of sixteen road tiles, indexed by which of its four neighbours it joins:

```cpp
if (paved(paths, step(cell, direction)))
{
    links = static_cast<std::uint8_t>(links | linkBit(direction));
}
```

`roadTile(links)` is then `atlasSlot(kFirstRoadSlot + (links & kLinkMask))` — a lookup, not four decisions, because there is exactly one road tile per link mask.

The mask is worked out in the scene, from the snapshot's paths, which arrive in ascending order — so asking whether a neighbour is paved is a binary search rather than a second index to keep in step with the first.
And it stays out of `SceneSnapshot` and `GameSummary` entirely.
Which tile a road shows is a *picture*, not state a replay has to reproduce, and the moment it appears in a snapshot it becomes something two runs could disagree about.

## The picture is generated, and that is the point

`src/apps/game/assets/atlas.png` is committed, but it is not hand-painted.
`scripts/generate_game_atlas.py` draws it, and the reason is not that nobody could draw.

A tile's art has to agree with the projection the game blits it through.
A road stub has to run along the diamond's axes; the ground tile's rim has to sit where the cell's edge is; a walker has to point the way the game says it is facing.
Painted by hand, those are four opportunities for the art to be nearly right, and "nearly right" in an isometric projection reads as a seam that appears at one zoom level and not another.

So the art is drawn in grid space.
The generator inverts the same projection per pixel:

```python
east = (dx / (TILE_WIDTH / 2) + dy / (TILE_HEIGHT / 2)) / 2
south = (dy / (TILE_HEIGHT / 2) - dx / (TILE_WIDTH / 2)) / 2
```

Everything after that asks about `east` and `south` rather than about pixels.
Inside the cell is `abs(east) <= 0.5 and abs(south) <= 0.5`.
The lattice is `max(abs(east), abs(south)) > 0.5 - RIM`.
A road stub running east is a band in grid space, and its on-screen shape falls out of the projection rather than being drawn to match it.

## The slot numbers are read out of the header

The interesting decision is not that the art is generated — it is *where the layout comes from*.

The obvious version has the slot numbers twice: once in `TileAtlas.hpp` where the game addresses them, once at the top of the generator where it draws them.
Two sets of numbers that must agree, with nothing making them.

So the generator parses them out of the header instead:

```python
# The layout comes from these headers, the ones beside this script.
# --root says where the picture goes, not what it is drawn from.
```

It reads `kAtlasTileSize`, `kAtlasColumns`, `kAtlasRows`, `kGroundSlot`, `kFirstRoadSlot`, `kRoadSlotCount`, and the `Direction` enumerators in declaration order — because declaration order is what `linkBit()` shifts by, so a road's bit ordering travels the same way its slot numbering does.

The parsing is deliberately narrow.
An integer constant must be a literal:

```python
# Only a literal, never an expression.
# A constant the header derives is derived here the same way.
```

A `Size` must be a designated-initialiser literal with `.width` and `.height`.
That means renaming or rewriting one of those declarations does not silently fall back to a default — it fails the generator loudly, with the name it could not find.
Matching by name *and* by shape is what turns a refactor in the header into a red build rather than a quietly wrong picture.

## Three ways a wrong layout fails before it reaches a screen

The same layout facts are asserted in three places, and each catches something the others cannot.

In the header, as `static_assert`, because every number there is `constexpr`:

```cpp
static_assert(
    kRoadSlotCount == 1U << kDirectionCount,
    "there must be a road tile for every link mask");
```

The comment above them says why they exist at all: *on screen is the only other place it could fail*.
There are three more like it — the walker slots have to fit in the atlas, `kDirectionCount` has to count exactly the named directions, and `kAtlasSize` has to be the grid of tiles the generator draws rather than a hand-typed number that agrees today.

In the generator, as `check_layout()`, asking the same questions of what it parsed, plus the two it alone can ask: its per-facing colour and step tables are written out rather than derived, so a fifth direction would leave a walker undrawable.

And in CI, as drift:

```sh
python3 scripts/generate_game_atlas.py --check
```

which regenerates the image in memory and compares bytes.
A committed PNG that no longer matches its generator fails the build with `Stale:` and the command to fix it.

That last one is what makes the committed file honest.
It is in the repository so that a clone builds and runs without Python, and it is checked so that it cannot become a second source of truth about what the game's tiles look like.
