# apps/gfx_demo

`src/apps/gfx_demo/` — the graphics and UI showcase.

## What it demonstrates

[`gfx`](../libraries/gfx.md) and [`ui`](../libraries/ui.md) together, without the tick loop: bars, a texture blitted from a PNG, and a panel of clickable buttons painted over them.

## Running it

```sh
build/bin/antwika_gfx_demo/antwika_gfx_demo
```

It opens a window titled "Antwika gfx demo" and runs until the window is closed.
Under the default `null` backend it draws nothing and reports no close, so use an `sdl3` or `raylib` build — with `SDL_VIDEODRIVER=dummy` or `xvfb-run` if there is no display.

It takes no flags of its own, and it parses its command line all the same, against an empty [`cli`](../libraries/cli.md) table: `--help` prints and returns without opening a window, and anything else is a `CommandLineError` rather than a silently ignored argument.
That is the failure `cli`'s own page recounts fixing for [`sound_demo`](sound_demo.md) — a program that reads nothing accepts every typo, and looks completely normal while doing none of what it was told.

## Libraries it composes

[`app`](../libraries/app.md), [`cli`](../libraries/cli.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), plus the selected graphics and input backends.
Notably not [`engine`](../libraries/engine.md) or [`replay`](../libraries/replay.md): there is nothing to record here.

## How it is put together

`DemoScene` describes the picture and `DemoLoop` runs it.
The PNG is copied into the application's own directory under `bin/` at build time, found with `app::assetPath()` and read by `app::readPngFile()`, because `antwika::gfx` opens no files itself.
The panel is painted **last**, so it reads as being in front of the bars and the logo — the renderer has no z-order, so paint order is the only ordering there is.
Its two buttons count and reset a click counter the loop owns, which is the point: the counter lives in the application, not in `antwika::ui`, because the UI retains nothing between frames.

## Non-obvious decisions

**The pointer is an argument, not a device read.**
`app::pointerFrom()` folds `input`'s edges into a `ui::Pointer` that is handed to the `ui::Context`, so the UI library still reads nothing.

**It is the worked example for `ui::applyHover()`, and it earns that honestly.**
The buttons light up on approach without one byte entering a recording: `DemoLoop` publishes the free-moving position to an `input::PointerHintChannel`, `app::hoverFrom()` reads it as a `ui::HoverPointer`, and one `applyHover()` call repaints the frame after it was resolved.
Because this app has no recorder to gate the stream for it, `DemoLoop` gates the position out of its own `ui::Pointer` by hand, which is what makes the demonstration honest rather than circular — a hover drawn from a position the `Pointer` still carried would prove nothing.
See [`docs/hover-is-not-simulation.md`](../../docs/hover-is-not-simulation.md).

**Activation is on the press.**
There is no release-to-activate and no pointer capture, because either would be cross-frame state — see [`ui`](../libraries/ui.md).

See [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md).
