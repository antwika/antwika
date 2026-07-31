# The generated atlas was the wrong kind of correct

*Post 19*

[Post 17](017-the-atlas-is-generated-from-the-numbers-that-address-it.md) argued that `apps/game`'s texture atlas should be generated rather than painted.
The argument was good, and the code that came out of it worked exactly as described for as long as it existed.
It has now been deleted, and this post is about why an argument can be sound and still be answering the wrong question.

## What the generator was defending

The claim in post 17 was about precision, and it was true.

> A tile's art has to agree with the projection the game blits it through.
> A road stub has to run along the diamond's axes; the ground tile's rim has to sit where the cell's edge is; a walker has to point the way the game says it is facing.
> Painted by hand, those are four opportunities for the art to be nearly right, and "nearly right" in an isometric projection reads as a seam that appears at one zoom level and not another.

`scripts/generate_game_atlas.py` drew every tile in grid space, so a road stub's shape *fell out of* the same projection the game blits it through rather than being matched to it by eye.
It parsed `kAtlasColumns`, `kFirstRoadSlot` and the rest out of `TileAtlas.hpp` by name and by shape, so renaming a constant failed the generator loudly instead of drifting the picture quietly.
CI byte-compared the committed PNG against a fresh render, which meant the checked-in file could not become a second source of truth.

Three layers, each catching something real.
None of that was wrong.

## What it was not defending

It was defending correctness against the projection.
Nobody was attacking that.

What the atlas actually needed was to look like something, and a script that draws ellipses and shaded diamonds cannot be argued into that.
The generator's ground tile was a green diamond with a darker rim.
Its walkers were a coloured disc with a smaller disc for a nose.
Its buildings were a footprint, a wall and a roof, sized so the ridge stayed inside the cell.

Every one of those is *exactly right* and none of them is art.
The precision the generator bought was real, and it was being spent defending a placeholder.

That is the shape of the mistake, and it is worth naming because it is not a bug: **the guarantee was sound, the thing it guaranteed was not worth guaranteeing yet.**
A generated atlas is a good answer to "how do I keep programmer art consistent with the projection".
It is not an answer to "how does this game get art", and the second question was the one in front of us.

## What survives

`TileAtlas.hpp` is unchanged, and that is the point.
It was always the address map — slot arithmetic rather than a table of rectangles, so there is no list in the header that could disagree with the picture — and it still is.
Its four `static_assert`s still fire on a layout that does not fit.
`TileAtlasTest` still pins the slot ranges and their non-overlap.

What is gone is the generator, its test, and the two CI steps that ran them.
The PNG beside the header is now the source of truth for what the game looks like, and editing it is editing the art.

The trade is stated plainly, because it is a real one.
Repainting a tile is now free, and nothing has to be re-run.
*Moving* one is not free, because the header is what decides which pixels a slot means, and nothing checks that the art agrees with it any more.

## The one check that had to be added back

Deleting the generator quietly removed a guarantee nobody had written down: while the picture was generated, its dimensions came from the same constants that address it, so the file could not be the wrong size.

Hand-drawn, it can be — and the symptom is horrible.
`gfx::blitIsDrawable()` refuses a source rectangle reaching outside its texture, and it refuses it *silently*.
An atlas exported one row short does not fail, does not log, and does not crash.
It draws a blank grid.

That is precisely the failure [post 18](018-a-run-that-completes-and-is-still-wrong.md) is about, so it gets the treatment post 18 prescribes: refused at the boundary, once, with a message naming both sizes.

```cpp
void requireAtlasSize(const antwika::gfx::Bitmap &bitmap);
```

It lives in `game::AtlasImage` rather than in `main.cpp`, because `main.cpp` is branchless by rule and because a check behind a seam is a check a test can reach without an image on disk.
Five tests cover it, none of which opens a file.

## The rule

A guarantee has a cost, and the cost is paid in what it forecloses.
The generator's cost was that the game could only ever look like something a script could draw, and that cost was invisible for as long as nobody wanted it to look like anything else.

So the question to ask of a mechanism like this is not "is it correct" — the generator was — but **"what does it make impossible, and do I want that thing?"**

Post 17 is left standing rather than edited.
It records what was true when it was written, and the reasoning in it is still the right reasoning for the problem it was solving.
This post is what happens when the problem changes.
