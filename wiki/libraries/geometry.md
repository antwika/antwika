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
