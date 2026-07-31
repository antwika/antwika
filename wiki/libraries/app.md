# antwika::app

`src/libs/app/` — the wiring every `main.cpp` would otherwise repeat.

## What it is for

The composition-root helpers shared by the applications: parsing the shared flags, standing up logging, running a recorded or replayed session, reading an asset from disk, and folding input edges into a `ui::Pointer`.
It holds no simulation logic of its own — everything here is glue that was duplicated across three or more `main.cpp` files before it was extracted.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `RunRecorded.hpp` | `RecordedRun`, `runRecorded()`, `scriptedEvents()` | Parses `--record`/`--replay`, builds the source, runs the loop through a caller-supplied body, and writes the file when the run ends. |
| `RunGuarded.hpp` | `runGuarded()` | Runs a body, reports what it threw under the program's name, and returns an exit code. |
| `ConsoleLogging.hpp` | `ConsoleLogging` | A `Logger` over `std::ostream` at a minimum `Level`, exposed as `logger()`. |
| `PngFile.hpp` | `readPngFile()` | Reads a PNG path into a `gfx::Bitmap`. |
| `PointerReading.hpp` | `asPoint()`, `locates()`, `pointerFrom()` | Turns `input` edges into a `ui::Pointer`. |

## Depends on

[`event`](event.md), [`gfx`](gfx.md), [`input`](input.md), [`log`](log.md), [`replay`](replay.md), [`time`](time.md), [`ui`](ui.md).
It is the widest library in the project, and that is the point: it is where the seams are joined, so no other library has to know about more than its own neighbours.

## Non-obvious decisions

**`readPngFile()` lives here, not in `gfx`.**
`antwika::gfx` opens no files at all — `PngReader::read()` takes a byte stream — so that every decoder failure is provable headlessly and no backend ever receives a path.
Opening the file is an application concern, so the helper that does it sits in the application layer.

**`PointerReading` is the bridge `ui` refuses to build.**
`antwika::ui` reads no device: a `ui::Pointer` arrives as an argument.
`pointerFrom()` is where an application folds `input`'s edges into one, and `locates()` is the predicate for whether an event carries a position at all — which matters because `input.pointer_scroll` does not.
`asPoint()` converts an `input::Position` to a `gfx::Point`, the one place the deliberate duplication between those two types is resolved.

**`runGuarded()` is the half of `runRecorded()` that knows nothing about replays.**
It exists because an app's `main.cpp` may not have a `try` of its own, and `gfx_demo` and `gfx3d_demo` take no `--record`/`--replay` to call `runRecorded()` with.
`runRecorded()` calls it twice -- once around the parse and the body, once around the save -- which is what lets a failed run still write what it recorded, and an unwritable `--record` path be reported rather than thrown out of a `main()` that cannot catch it.
A throw that is not a `std::exception` is deliberately let through: that is a bug in the body rather than a failed run.

**`runRecorded()` takes the app's body as a callback.**
The app supplies the sinks, the systems and the self-generated event names; the helper owns the order — source, recorder, then app sinks — because that order is what the recording rule depends on and is not something each `main.cpp` should get right independently.

See [`blog/009-json-wins-tickevent-and-three-mains-that-stopped-repeating-themselves.md`](../../blog/009-json-wins-tickevent-and-three-mains-that-stopped-repeating-themselves.md).
