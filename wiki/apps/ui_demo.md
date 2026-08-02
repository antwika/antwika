# apps/ui_demo

`src/apps/ui_demo/` — every `antwika::ui` element, a page at a time.

## What it is

`apps/ui_demo` is the showcase for [`ui`](../libraries/ui.md): labels, buttons, nested layouts, a text field, dropdowns, the focus ring, the theme, where a named widget was laid out, and what a container does with less room than its children want.
A dropdown at the top of the window picks which of those pages is showing, and that picker is itself one of the elements being shown.

The nine pages are the `ui_demo::Showcase` enumeration in `Showcase.hpp`, and the names the picker lists are one array in the same header, in the same order.
How many pages there are is derived from the last enumerator rather than written down, so a page added to the enumeration gains its option without a second list that could drift from the first.

Unlike [`gfx_demo`](gfx_demo.md), which draws a UI without a tick loop at all, this one is an ordinary application of the tick loop: it records, it replays, and every widget it draws is resolved inside the tick path.

## Running it

```sh
build/bin/antwika_ui_demo/antwika_ui_demo
build/bin/antwika_ui_demo/antwika_ui_demo --record demo.replay
build/bin/antwika_ui_demo/antwika_ui_demo --replay demo.replay
```

It opens a 960x720 window titled "Antwika ui demo" and ends after 1500 ticks, which at its 40 ms frame period is about a minute.
Under the default `null` backend it draws nothing and takes no input, so use an `sdl3` or `raylib` build — with `SDL_VIDEODRIVER=dummy` or `xvfb-run` if there is no display.

## Libraries it composes

[`app`](../libraries/app.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`gfx`](../libraries/gfx.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`simulation`](../libraries/simulation.md), [`time`](../libraries/time.md) and [`ui`](../libraries/ui.md), plus the selected graphics and input backends.

## How it is put together

- `DemoState` holds everything the showcase remembers between frames.
- `DemoSink` folds this tick's input, describes the UI through `DemoScene`, resolves it, and applies what came back.
- `DemoOverlay` is the one thing the tick path and the renderer share: `DemoSink` writes the picture into it once per tick and `RenderSink` paints it, so the renderer never has to know what a pointer or a widget is.
  It also owns the canvas the UI is laid out against, which is the size the window was *asked* for rather than the size a window reports, so nothing can lay the showcase out against one size and hit-test it against another.
- `KeyMapping` turns a key edge into the `ui::Key` the library acts on.
- `app::TickLimitSource` is what ends the run.

## Non-obvious decisions

**The application owns every byte of widget state, because `antwika::ui` retains nothing.**
Which page is showing, whether either dropdown is open, what each has selected, the field's characters and caret, which widget has focus and the click counter all live in `DemoState`.
Each of them is handed *in* to a `ui::Context` and handed back *out* of the `ui::Frame` it produces.
That is not an inconvenience the demo works around — it is the property being demonstrated, since state the library kept would be state a replay could not regenerate from the recorded clicks and keystrokes.

**The UI is described and resolved inside the tick path, downstream of the recorder.**
`DemoSink` does all of it, never `RenderSink`, so what a recording holds is the click and the key press, and which widget they landed on is worked out again on replay.
Persisting "chose page 4" beside the click that chose it would apply the choice twice, which is why **no `ui.*` event name exists here** any more than in [`game`](game.md) or [`tower_defence`](tower_defence.md).

**Which key means what is the application's decision, and the character it types is written down rather than asked of a window system.**
`uiKeyFor()` maps Tab, Shift+Tab, Enter and Escape onto `ui::Key::FocusNext`, `FocusPrevious`, `Activate` and `Cancel`; `typedCharacterFor()` maps a key and the shift state onto a character.
A recording therefore holds the symbolic key, and the same character comes back under any backend on any keyboard — the same reason `InputEventCodec` persists key names rather than scancodes.
Escape reaching the UI as `Cancel` is the one place this parts company with `apps/game`: nothing here stops on Escape, so a field can be given up on, which is all a showcase of `ui::TextEdit::cancelled` needs.

**Pointer movement is coalesced but deliberately not gated.**
`apps/game` attaches `input::IdleMotionSource` as well, which holds idle movement back and costs it a button that lights up on approach.
A showcase of a hoverable widget cannot pay that, so this app takes the coalescing decorator only: one position per tick is all a layout reads, and the hovered appearance is `antwika::ui` resolving the recorded event stream rather than anything off `input::PointerHintChannel`.

**Running out of budget is an ordinary stop, not a loop that failed.**
The default `null` backend reports no window close, and that is the build every CI leg produces, so a run left to end on a close would never end there.
`app::TickLimitSource` appends `engine.stop` from its budget's tick onwards rather than using `EngineLoop`'s `maxTicks`, which throws.
Saying so as an event also puts the ending upstream of the recorder, exactly as `poker::WindowCloseSource` puts a window close there, so a `--record` run ends its file on the tick it stopped on and replaying that file stops on the same one.
`UiDemoConfig::maxTicks` remains as the safety cap a test sets.

**The picker's options are a static array.**
`ui::DropdownSpec` borrows the options it is given, so a vector built per frame would be gone before the `ui::Context` it was handed to laid anything out.
