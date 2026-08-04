# antwika::geometry

`src/libs/geometry/` — points, sizes and rectangles, and nothing else.

## What it is for

Three value types, header-only, depending on nothing.

They lived in [`gfx`](gfx.md), which meant every module wanting a width and a height linked a graphics library to get one.
[`replay`](replay.md) records the canvas a session was made against, so it pulled in stb, glm and an embedded TrueType font for the sake of two integers -- and [`config`](config.md) inherited the whole graph through it.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Point.hpp` | `Point` | An `x` and a `y`. |
| `Size.hpp` | `Size` | A `width` and a `height`. |
| `Rect.hpp` | `Rect` | An origin and a size. |
| `Grid.hpp` | `Grid`, `GridCell`, `gridFit()`, `cellAt()`, `cellRect()` | Fitting square cells into an area, and reading a point back as one of them. |

## Depends on

Nothing.

## Non-obvious decisions

**`gfx::Point`, `gfx::Size` and `gfx::Rect` still name these types**, through a `using` in a header of the same name.
Two hundred and seventy-eight files call them that, and a rename would have been churn with nothing at the end of it; the point of the move is the *dependency*, not the spelling.
`gfx/Rect.hpp` re-exports the other two as well, because a caller that included it used to get them with it.

**A module that wants the honest name uses it.**
`replay` names `geometry::Size` outright, which is what lets its `CMakeLists.txt` drop `antwika::gfx` -- the whole reason the library exists.
Nothing forces an application to follow: `gfx::Size` is not deprecated, and code that is already talking to a renderer is not lying by using it.

**Header-only, on [`time`](time.md)'s terms.**
A value type whose only behaviour is comparing equal has nothing to compile.

**`Grid.hpp` is the one header here with arithmetic in it, and it is here because three applications had a copy of it.**
[`life`](../apps/life.md), [`sudoku`](../apps/sudoku.md) and [`tower_defence`](../apps/tower_defence.md) each fitted the largest whole-pixel square cell into an area, centred what fitted, and read a point back as a cell -- the same twenty lines with the same two comments on them.
Each copy carried the same two decisions: a cell is a whole number of pixels, so nothing is ever half a pixel wide; and the hit test widens to 64 bits before subtracting the origin, so a pointer far outside the area misses rather than wraps.
Both are the kind of thing that is right in three places or wrong in one.

Each application keeps its own layout type all the same -- `life::BoardLayout`, `sudoku::BoardLayout`, `tower_defence::GridLayout` -- because what a cell *means* differs and a `sudoku::Square` is not a `tower_defence::Cell`.
What moved is the arithmetic, not the vocabulary.
`tower_defence` folds in too, despite taking the score bar's height off the top first: taking it off is choosing a smaller `Rect` to fit into, which is what makes a click on the bar fall outside the grid, and the fitting itself is the same fitting.

It is still true that this library depends on nothing, which is what made it the only place all three could reach.
