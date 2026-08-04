# antwika::console

`src/libs/console/` — the debug console every application mounts, and the snapshot seam behind its two commands.

## What it is for

A tilde-toggled console that slides over any application's screen, takes a typed command line, and executes `dump_state`/`load_state` against a seam the application implements.
It began as `apps/game`'s own feature and became a library the day every other application wanted it; the game is still the richest consumer, and its wiki page documents the app-side behaviour in full.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `ConsoleState.hpp` | `ConsoleState`, `consoleHeightAt()`, `kConsoleAnimTicks` | The slide, the field, the history — all simulation state, tick-counted. |
| `ConsoleScene.hpp` | `ConsoleScene`, `consoleWidgets`, `kConsoleHistoryShown` | Describes the sheet as a `ui::Frame`, bottom-anchored field included. |
| `ConsolePicture.hpp` | `ConsolePicture` | The picture the sink writes and a renderer paints last, with the canvas it was described against. |
| `ConsoleSink.hpp` | `ConsoleSink`, `ConsoleSinkSetup` | Resolves the toggle, the typing and the execute press inside the tick path. |
| `ConsoleGatedSink.hpp` | `ConsoleGatedSink` | Wraps any sink; what the console stands over, it takes. |
| `InputFold.hpp` | `InputFold` | The one fold of `input.*` events every console-mounting app registers first. |
| `IConsoleControls.hpp` | `IConsoleControls`, `FixedConsoleControls` | The toggle key, the execute key and the typing board — a seam, since the game answers off rebindable options. |
| `IConsoleCommands.hpp` | `IConsoleCommands` | What an executed line does; the sink knows no command by name. |
| `SnapshotCommands.hpp` | `SnapshotCommands`, `consoleLoadPermitted()` | The dump_state/load_state policy, written once for every app. |
| `ISnapshotStore.hpp` | `ISnapshotStore` | The application's half: what its state is, taken and applied. |
| `SnapshotFormat.hpp` | `Snapshot`, `SnapshotFormat` | The shared dump envelope under each app's own magic, version and migrations. |
| `SnapshotError.hpp` | `SnapshotError` | The one failure category this library owns. |
| `KeyboardLayout.hpp`, `Typing.hpp` | `KeyboardLayout`, `typedCharacterFor()`, `consoleKeyFor()` | Which physical board a key position types by — English and Swedish QWERTY, ASCII only. |
| `PointerReading.hpp` | `asPoint()`, `locates()` | The input-position-to-canvas-point crossing, said once. |

## Depends on

[`animation`](animation.md), [`config`](config.md), [`engine`](engine.md), [`event`](event.md), [`gfx`](gfx.md), [`input`](input.md), [`replay`](replay.md), [`time`](time.md), [`tween`](tween.md), [`ui`](ui.md).

## Non-obvious decisions

**The console defines no event, and no `console.*` name may ever exist.**
The toggle key, the typing and the execute press are recorded input; the slide, the history and every command's effect are regenerated inside the tick path, downstream of the recorder — exactly as a click regenerates the tile it laid.

**The slide is tick-counted simulation state shaped by a tween.**
Whether the console is open decides what a key press means, so how far along the slide is has to be something a replay reaches again: `ConsoleState` counts `kConsoleAnimTicks` whole ticks, and `consoleHeightAt()` shapes the count with `tween::Easing::CubicOut` — exact rational arithmetic, the same pixel on every toolchain.
The field reads only fully open, which is what keeps every keystroke's meaning a function of state rather than of a frame.

**The console belongs to no mode and pauses nothing.**
A debugging surface has to be reachable from whichever screen the thing being debugged is on, and an application keeps running underneath — the game's city walks on under an open console, and so does every other app's simulation.

**`ConsoleGatedSink` is a decorator, for `ModeGatedSink`'s reason.**
While any of the sheet is out, every key edge is the console's, and a press or a scroll above its bottom edge is too; movements and releases pass, so a drag begun below carries across.
"The console covers this" is stated once, where a sink is registered, and a sink under it needs no opinion about consoles at all.
Everything the gate reads is folded from recorded input, so a replay gates identically by construction.

**dump_state runs everywhere; load_state only in a plain live run.**
A dump is a write-only projection of state a replay reproduces, so a replayed run re-executes it and rewrites the same file — which doubles as a way of reading any tick's state out of a recording.
A load reads a file whose contents no recording carries, so under `--record` or `--replay` it answers with a deterministic refusal line instead; `consoleLoadPermitted()` is that rule, and the refusal being an ordinary history line is what keeps a hand-authored replay reading exactly what a recorded run would have.

**One envelope, one magic per application.**
Every app's dump file is `{magic, version, console, state}`: the console's history is carried by the envelope so that coming back to an instant means reading what it read, and the `state` member is opaque here — each application validates and decodes its own behind `ISnapshotStore`, with its own magic, version and migration chain, so one app's dump refuses to load into another on the magic alone.

**Every word the console says is a literal, not a message id.**
A command language is a format, like a save file's kind names, and history lines are state a dump carries — wording them per locale would make a dumped console a function of the language it was typed under.

**The typing tables are ASCII only, keyed by position.**
`input::Key` says where a key *is*; `typedCharacterFor()` says what that position prints on the chosen board.
The Swedish letters would be multi-byte in the UTF-8 the UI holds, and a field's caret arithmetic counts bytes, so å, ä and ö type nothing rather than something a caret would land inside.

## Who uses it

Every application except the four demos; each one's wiki page describes its own snapshot store — what its `state` object carries, and what it deliberately leaves out.

## The store half is written once too

`JsonSnapshotStore<ErrorT>` is the `ISnapshotStore` an application inherits rather than implements, and every store in the tree now does.
It holds the `SnapshotFormat`, writes `{console, state}` to a path, reads one back, and asks the application for the two halves that are actually its own -- `takeState()` and `applyState()` -- which is all that ever differed between nine copies of the same twenty lines.

Both halves are handed the document's path, because a store's state need not all fit inside the document.
[`atlas_editor`](../apps/atlas_editor.md) writes its sheet and its clipboard as PNGs beside it and binds them to it by fingerprint, and that is what the parameter is for.

**`ErrorT` is the failure category the store rewraps, and the narrowness is the point.**
It is parameterised for `config::FileFormat`'s reason: what differs between two applications reading a versioned document is which error each one reports.
A store names what its own state can be wrong about, that becomes the `SnapshotError` `ISnapshotStore` documents, and a failure from further down -- [`pattern`](pattern.md)'s refusal to place a segment under [`music_editor`](../apps/music_editor.md), say -- is not that and travels on as itself.
A store whose state's own reader already refuses with `SnapshotError` names that as its `ErrorT`, and the rewrapping is then the identity.

What the envelope's own write throws is outside the rewrapping either way: it is already a `SnapshotError`, and a full disk is the machine's truth rather than the state's.
