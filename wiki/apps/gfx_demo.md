# apps/gfx_demo

`src/apps/gfx_demo/` — the graphics and UI showcase.

## What it demonstrates

[`gfx`](../libraries/gfx.md) and [`ui`](../libraries/ui.md) together, without the tick loop: bars, a texture blitted from a PNG, and a panel of clickable buttons painted over them.

## Running it

```sh
build/bin/antwika_gfx_demo
```

It opens a window titled "Antwika gfx demo" and runs until the window is closed.
Under the default `null` backend it draws nothing and reports no close, so use an `sdl3` or `raylib` build — with `SDL_VIDEODRIVER=dummy` or `xvfb-run` if there is no display.

## Libraries it composes

[`app`](../libraries/app.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), plus the selected graphics and input backends.
Notably not [`engine`](../libraries/engine.md) or [`replay`](../libraries/replay.md): there is nothing to record here.

## How it is put together

`DemoScene` describes the picture and `DemoLoop` runs it.
The PNG path is baked in at configure time and read by `app::readPngFile()`, because `antwika::gfx` opens no files itself.
The panel is painted **last**, so it reads as being in front of the bars and the logo — the renderer has no z-order, so paint order is the only ordering there is.
Its two buttons count and reset a click counter the loop owns, which is the point: the counter lives in the application, not in `antwika::ui`, because the UI retains nothing between frames.

## Non-obvious decisions

**The pointer is an argument, not a device read.**
`app::pointerFrom()` folds `input`'s edges into a `ui::Pointer` that is handed to the `ui::Context`, so the UI library still reads nothing.

**Activation is on the press.**
There is no release-to-activate and no pointer capture, because either would be cross-frame state — see [`ui`](../libraries/ui.md).

See [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md).
