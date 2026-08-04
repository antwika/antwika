# antwika::app

`src/libs/app/` — the wiring every `main.cpp` would otherwise repeat.

## What it is for

The composition-root helpers shared by the applications: parsing the shared flags, standing up logging, running a recorded or replayed session, finding and reading an asset shipped beside the executable, drawing the frames between two ticks, and folding input edges into a `ui::Pointer`.
It holds no simulation logic of its own — everything here is glue that was duplicated across three or more `main.cpp` files before it was extracted.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `RunRecorded.hpp` | `RecordedRun`, `runRecorded()`, `scriptedEvents()` | Parses `--record`/`--replay` and each app's own flags in one pass, opens the recording, and runs the loop through a caller-supplied body. |
| `RunGuarded.hpp` | `runGuarded()` | Runs a body, reports what it threw under the program's name, and returns an exit code. |
| `ConsoleLogging.hpp` | `ConsoleLogging` | A `Logger` over `std::ostream` at a minimum `Level`, exposed as `logger()`. |
| `AssetPath.hpp` | `executableDirectory()`, `assetPath()` | Where the running program is, and where a file shipped beside it is. |
| `PngFile.hpp` | `readPngFile()` | Reads a PNG path into a `gfx::Bitmap`. |
| `WavFile.hpp` | `readWavFile()` | Reads a WAV path into a `sound::Waveform`; `readPngFile()`'s line-for-line counterpart. |
| `FramePacedSource.hpp` | `FramePacedSource`, `FramePacing` | An `ITickEventSource` decorator that draws `framesPerTick - 1` frames in the gap before each tick's events are read, and paces the tick through an injected `time::ISleeper`. |
| `IFramePass.hpp` | `IFramePass` | `draw(animation::Progress)` — what one of those frames is handed, and all it is handed. |
| `FramePacingError.hpp` | `FramePacingError` | A pacing no loop could honour, such as a tick that draws no frames at all. |
| `FramePresentation.hpp` | `Pictured`, `drawsOn()`, `paintOver()`, `presentFrame()`, `presentViewport()` | The un-paced frame: the tick-and-open guard, the overlay painted last, the present, and the viewport a fixed canvas is placed through. |
| `PointerReading.hpp` | `asPoint()`, `locates()`, `pointerFrom()`, `hoverFrom()` | Turns `input` edges into a `ui::Pointer`, and a pointer hint into a `ui::HoverPointer`. |
| `WindowPointerMapping.hpp` | `WindowPointerMapping` | An `input::IPointerMapping` reading a window pixel as a pixel on the fixed canvas the app lays out against. |
| `FullscreenToggleSource.hpp` | `FullscreenToggleSource` | An `ITickEventSource` decorator making a nominated key fill the screen with the window, altering not one event. |
| `TickLimitSource.hpp` | `TickLimitSource` | An `ITickEventSource` decorator that appends `engine.stop` from a chosen tick onwards, so a capped session ends as recorded input rather than a thrown `maxTicks`; `MaxTicks.hpp` is the `--max-ticks` flag name and its zero-means-no-cap parse. |
| `WindowCloseSource.hpp` | `WindowCloseSource` | An `ITickEventSource` decorator that closes the window a close request names and appends `engine.stop` while it is shut, with the pump exposed for a loop drawing after the run. |

## Depends on

[`animation`](animation.md), [`cli`](cli.md), [`event`](event.md), [`gfx`](gfx.md), [`input`](input.md), [`io`](io.md), [`log`](log.md), [`replay`](replay.md), [`simulation`](simulation.md), [`sound`](sound.md), [`time`](time.md), [`ui`](ui.md).
It is the widest library in the project, and that is the point: it is where the seams are joined, so no other library has to know about more than its own neighbours.

## WindowInputSource

**Turning a window's close request into the `engine.stop` that ends a run** lives here rather than in [`simulation`](simulation.md), because it is the one thing in that library that had to know what a window is.
Moving it took `gfx` out of the tick loop's dependency list entirely.

## The two things every app kept writing twice

`storeIfLive()` is **the guard that keeps a replay from writing**: reading a machine's file would resolve a recorded session against state the recording never carried, and writing it would leave whoever replayed somebody else's session carrying its result.
`companion` and `tower_defence` had a copy each, untested in both; it is one function with one test now.

`FileSnapshotStore<ValueT, ErrorT>` is the file half of a snapshot -- a missing file is a first run rather than a corrupt one, and a write that cannot open or cannot land says so, through [`io`](io.md), which owns the open/flush/check discipline for the whole tree.
`FilePetStore` and `FileScoreStore` were the same fifty-five lines with seventeen names changed between them, and are thin forwarders now.

## Non-obvious decisions

**`readPngFile()` lives here, not in `gfx`.**
`antwika::gfx` opens no files at all — `PngReader::read()` takes a byte stream — so that every decoder failure is provable headlessly and no backend ever receives a path.
Opening the file is an application concern, so the helper that does it sits in the application layer.

**`readPngFile()`'s counterpart for audio is `readWavFile()`, and it is deliberately line for line.**
`antwika::sound` takes the same position about files that `antwika::gfx` does — `WavReader::read()` decodes a byte stream and never goes looking for one — so the two callers should not have to be read differently to see it.

**`assetPath()` asks the operating system where the program is.**
`/proc/self/exe` on Linux, `GetModuleFileNameW` on Windows: never the working directory, and never `argv[0]`, which holds whatever the caller happened to type and holds nothing useful at all when a program was found on `PATH`.
So starting an application from anywhere still works, and `antwika::app` is the one place under `src/` that names an operating system.
What it replaces is a path baked in at configure time, which was the *building* machine's path — right on the machine that built it, and a directory that does not exist on any other, so every cross-built executable that opened anything died on its first line.

**Every application gets a directory of its own under `bin/`.**
`antwika_bundle_app()` in [`cmake/AntwikaModule.cmake`](../../cmake/AntwikaModule.cmake) puts the executable there, along with whatever it opens and — on MinGW — the runtime DLLs it needs to start; `assetPath()` is how the running program finds those files again.
Two applications ship texture atlases and three a `demo.jsonl`, so one shared `bin/` was one set of atlases and one demo replay between all of them the moment either had to sit beside its binary.

A test binary goes to the directory of the module that owns it, put there by `antwika_bundle_test()` in the same file: `bin/antwika_companion/antwika_companion_tests` beside the application it covers, and `bin/antwika_replay/antwika_replay_tests` in a directory of the library's own.
That directory is the target's own name with the trailing `_tests` taken off rather than a second argument, so the name and the directory cannot disagree, and a target not following the convention is refused at configure time rather than landing somewhere surprising.
The same function registers the cases with CTest, because moving a binary and saying where it went are one decision, and leaving the second in every `tests/CMakeLists.txt` is exactly the drift one home for the rule prevents.
Test binaries still open nothing, so what a directory each buys is a `bin/` a reader can navigate rather than anything a binary finds beside it.

**A frame drawn between two ticks is handed nothing it could change.**
`FramePacedSource` decorates an `ITickEventSource` rather than changing [`simulation`](simulation.md)'s `EngineLoop`, and it is a pure observer of the stream in exactly `input::PointerHintSource`'s sense: `eventsFor()` hands back what the source it wraps returned, unchanged, so a recording is a function of a stream it cannot touch and attaching it is free rather than merely cheap.
It also replaces a `TickPacer` for the app that takes it, since one frame a tick is the same thing that did.

`IFramePass::draw()` is the other half.
Its one argument is an `animation::Progress` — how far through the tick the frame falls, as an exact rational rather than a float, so a picture asserted call by call is the same picture on every toolchain — and it is handed **no `World`, no `Tick`, no dispatcher and no event source**.
A pass between two ticks cannot change what the simulation computes because it is given nothing it could change, which is structural rather than a promise; an implementation therefore has to have been handed whatever it draws from before the frame began, which in practice means a snapshot taken on the tick.
`FramePacingError` is its own type, per the one-exception-type-per-failure-category rule, because a tick that draws no frames at all is a pacing nobody could have meant.

**`PointerReading` is the bridge `ui` refuses to build.**
`antwika::ui` reads no device: a `ui::Pointer` arrives as an argument.
`pointerFrom()` is where an application folds `input`'s edges into one, and `locates()` is the predicate for whether an event carries a position at all — which matters because `input.pointer_scroll` does not.
`asPoint()` converts an `input::Position` to a `gfx::Point`, the one place the deliberate duplication between those two types is resolved.

`hoverFrom()` is a second function beside `pointerFrom()` rather than one more field on it.
It reads a `std::optional<input::PointerHint>` — the free-moving position `input::PointerHintChannel` carries and no recording holds — as a `ui::HoverPointer`, which is the only thing `ui::applyHover()` takes.
The two positions come from different places and mean different things, and keeping them apart in the type system is what stops one being passed where the other belongs.

**`WindowPointerMapping` is the other end of `gfx::ViewportRenderer`, and it lives here for `PointerReading`'s reason.**
The renderer places a fixed-size canvas inside a window through `gfx::viewportFor()`; this runs the very same transform backwards on a pointer position, so a click lands on whatever the pointer is over whatever size the window is.
[`input`](input.md) does not depend on [`gfx`](gfx.md) and cannot name a window, `gfx` does not depend on `input` and cannot name a pointer, so the sentence saying the two describe one thing has to be said above both.

**What it hands back is what a recording will hold**, because `input::InputPipeline` attaches it upstream of the recorder.
That is the whole design rather than a detail: a session recorded on a window of one size replays on a window of any other, with no window geometry in the file — see [`docs/resizable-windows.md`](../../docs/resizable-windows.md).
It reads the reported size afresh on every call rather than capturing it, so dragging an edge mid-session is handled by the next click being mapped through the new size, and by nothing else.

**`FullscreenToggleSource` is where "fill the screen" belongs, and the reason is structural.**
A fullscreen toggle is an action on the window, not simulation state: it changes what `gfx::IWindow::size()` reports and changes nothing else, and nothing a replay reproduces may read that number.
A sink is downstream of the recorder and inside the tick path, where everything *is* a function of state a replay reproduces, so this sits above the loop instead — a pure observer of the stream in exactly `input::PointerHintSource`'s sense, handing back what its inner source returned, unchanged.
The key press is ordinary recorded input, so a replay of a session in which somebody pressed it fills the screen at the same tick and reaches the same state either way, which is the property worth having rather than an accident.

It holds an `IWindow &` rather than a `WindowId`, unlike `simulation::WindowInputSource`: that class holds an id precisely so it cannot close a window a renderer is still drawing into, and the only calls this one makes are `isFullscreen()` and `setFullscreen()`.

**`WindowCloseSource` is the variant of `simulation::WindowInputSource` that *does* close the window, and it lives here rather than there for that reason.**
The library's form holds a `gfx::WindowId` and notes a close request in a `bool` local to one `eventsFor()` call, which is the form [`blog/012`](../../blog/012-a-window-that-cant-talk-back.md) argues for and the one to reach for by default: a source that cannot close anything cannot leave the tick carrying the stop drawing into a closed window.
This one holds the `IWindow &` and calls `close()`, so the window's own open/closed state is what says the session is over — which is what an application needs when it goes on pumping and rendering *after* the loop has finished, long after that `bool` would have gone out of scope.
Holding the final frame up until somebody closes the window is exactly that, and it is why `pumpEvents()` is public here and is called from outside the tick.

The bill for that is one the application pays: the tick carrying the stop still runs to completion, so its render pass has to return early on a closed window, which is what `poker::TableRenderSink::render()` does.
`apps/poker` is the one application on this source, and it was the app copy that this is — everything else is on the library's `WindowInputSource`, which needs no such guard.

**`runGuarded()` is the half of `runRecorded()` that knows nothing about replays.**
It exists because an app's `main.cpp` may not have a `try` of its own, and `gfx_demo` and `gfx3d_demo` take no `--record`/`--replay` to call `runRecorded()` with.
`runRecorded()` calls it once, around the parse, the recording and the body, which is what lets an unwritable `--record` path be reported rather than thrown out of a `main()` that cannot catch it.

**There is no save epilogue any more.**
It used to call `runGuarded()` a second time around a `saveReplayFile()` at the end, so that a failed run still wrote what it had recorded — and a run nobody let finish still wrote nothing.
A `--record` run now opens the file before its first tick and dispatches into a `replay::ReplayRecorder`, which appends and flushes a line per event, so a run that failed and a run somebody killed have both already kept everything they got to.
The cost of that is where a bad path is reported: before the session rather than after it, which is the better end to find out at.
A throw that is not a `std::exception` is deliberately let through: that is a bug in the body rather than a failed run.

**`runRecorded()` takes the app's body as a callback.**
The app supplies the sinks and the systems; the helper owns the order — source, recorder, then app sinks — because that order is what the recording rule depends on and is not something each `main.cpp` should get right independently.

**`runRecorded()` is also the one place the flag tables are concatenated and parsed.**
A program parses once, against one table: an argument not in it is a `cli::CommandLineError`, so a second pass would refuse whatever the first accepted — which is how `apps/poker`'s `--tick-delay-ms` once stopped working.
That is why each layer offers a *table* and a reader rather than a parser of its own — `replayCliFlags()`/`replayCliOptionsFrom()`, `game::saveCliFlags()`/`saveCliOptionsFrom()`, `poker::watchFlags()`/`watchOptionsFrom()` — and why this helper takes an app's own flags as a `std::span<const FlagSpec>` and reads them in the same pass as `--record` and `--replay`.

See [`blog/009-json-wins-tickevent-and-three-mains-that-stopped-repeating-themselves.md`](../../blog/009-json-wins-tickevent-and-three-mains-that-stopped-repeating-themselves.md).

## WindowedSession

**The fifty lines every windowed application opened with**, now stated once: announce both backends, open the window, seed a `replay::ReplaySource` from `--replay` or a demo recording, assemble the `input::InputPipeline` and put a `WindowInputSource` over it.
An application says only what it differs by, through a `WindowedSessionDesc`, and wires the bundle into its own `bootstrap()`.

**`readsDevice` is derived rather than passed.**
It is `!replayPath.has_value()` in one place instead of seven, because a replay already holds the input it recorded and reading a device too makes every event arrive twice — the one line of that preamble whose getting wrong shows up as a doubled recording rather than as a failure.

**It names nothing of [`console`](console.md)**, which depends on this library rather than the other way round, so a `console::ConsolePicture` here would be a cycle.
Each application still constructs its own, one line, and `canvas()` is what it is constructed against — so the picture is still laid against the size the window was *asked* for and never against one a window reports, which is the property that mattered.

**It creates no backend and owns no logging, deliberately.**
`gfx::makeSelectedBackend()` and `input::makeSelectedInputBackend()` are declared here and *defined* under [`backends/`](../../backends/), linked by an application rather than by a library, so a library calling one would drag a graphics framework into every test binary that links `antwika::app` — and a unit test of this class would open a real window under an `sdl3` build.
Taking the logger and both backends as constructor references is what keeps it provable against `MockGfxBackend` and `FakeInputBackend` instead.

Every field of `input::InputPipelineOptions` is passed through untouched except those two, so `apps/life` still turns coalescing off — a drag toggles every cell it crosses — and `apps/atlas_editor` still attaches neither thinning decorator.
`mapsPointerToCanvas` is a flag rather than a `WindowPointerMapping` because the mapping needs the window this description has not opened yet.

**Four applications are deliberately not on it**, and the reason is the same in each: bending either end to fit would cost more than the repetition does.
[`music_editor`](../apps/music_editor.md) announces a third backend in the same line and has to open a sound device between the input backend and that line; [`poker`](../apps/poker.md) opens no window of its own, logs no such line and runs at `Level::Warning`; [`game`](../apps/game.md) words its line differently and interleaves its preamble with three atlases, a `LocaleState` and a pointer hint channel; and [`sound_demo`](../apps/sound_demo.md) has no window, no tick loop and no replay at all.

One ordering detail is worth knowing: `apps/life` and `apps/companion` announce how to stop *after* the session rather than between the backend line and the window, since `drawsNothing()` is what they ask.
Creating a window emits nothing at `Level::Info` — the `sdl3` backend logs it at `Level::Debug` and the `null` one logs nothing — so what reaches the console is unchanged.
## FramePresentation is the un-paced cousin of FramePacedSource

`FramePacedSource` and `IFramePass` are about the frames *between* two ticks; `FramePresentation.hpp` is about the frame the tick itself draws, which ten render sinks and render systems were each writing out by hand.
The body they shared is four decisions rather than four lines -- draw only on `engine.tick`, draw only into an open window, paint the console's sheet *after* everything else, and present -- and the third is the one worth owning centrally: "last" is not a property any single call site can check, and an overlay painted a line too early is a bug that only shows up as a sheet with a toolbar drawn through it.

`presentFrame()` takes the frame body as a callback and paints the overlay after it returns, so the ordering is structural rather than remembered.
`presentViewport()` is the same shape for an application drawing a fixed canvas into a window of some other size: it is the one place the *reported* size is read, it builds a fresh `gfx::ViewportRenderer` per frame, and it fills the surround after the picture so a sprite reaching past the canvas's edge is covered rather than left showing in the bar.
Neither hands the body anything but a renderer, on exactly `IFramePass::draw()`'s terms -- there is nothing there to read a world from or write one to.

**The overlay is a `Pictured` concept rather than an interface**, satisfied by anything answering `commands()` with a `ui::DrawList`, which is what lets this header name neither [`console`](console.md) nor any application's own overlay type.
The two overloads of `paintOver()` are the whole reason it is a concept and not one signature: some applications hold their console as a plain reference and some as a `std::optional<std::reference_wrapper<const T>>`, because a console is only mounted when one was asked for, and the second overload is where "no console mounted paints nothing" is stated once instead of in four `if (has_value())` blocks.

**`drawsOn()` is deliberately not inside `presentFrame()`.** Three of the ten -- `life`, `task_worker` and `game` -- check neither the tick nor the window: they are `ecs::ISystem`s, so they only ever run on a tick, and window lifetime belongs to a composition root that keeps the window open for the whole run.
A guard that can never be false is a branch the coverage gate would demand an impossible test for, which is [`blog/012`](../../blog/012-a-window-that-cant-talk-back.md)'s point, so the guard stays a separate call the six sinks that need it make and the systems do not.
`poker` splits it further still, since `TableRenderSink::render()` is called from outside the tick as well.
