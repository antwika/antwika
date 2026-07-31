# A run that completes and is still wrong

*Post 18*

This project's worst bugs have a shape.
Nothing throws.
Nothing is logged.
The program starts, does something plausible, and stops with a zero exit code — and the thing it did is not the thing it was asked to do.

A crash is a gift by comparison: it has a stack trace and a line number.
The failures below all produced a complete, believable run, and every one of them was found by somebody wondering why a result looked slightly off.
This post is about the four that got fixed together, and about the rule that came out of them.

## The one the recording could not tell you about

[Post 13](013-the-camera-is-simulation-state.md) established that a recorded click is a pixel, and that which cell it means is a function of the camera.
[Post 15](015-a-toolbar-with-no-events-of-its-own.md) added that which *button* it means is a function of the canvas the UI was laid out against.

Both are settled inside the tick path, so both replay correctly — as long as the canvas is the same.
`apps/game` even has that constant in a header of its own, with the warning attached:

> Changing this number invalidates every existing recording.
> Which button a recorded click hits is a function of the layout, and the layout is a function of the canvas, so a click near a button's edge lands on its neighbour -- or on the grid -- under a canvas the recording was not made against.
> The file still parses and the run still completes; only the outcome differs.

That last sentence is the whole problem.
The file is valid.
The events are all there, in order, with their payloads intact.
The replay runs to its recorded stop and reaches a different state, and nothing anywhere is in a position to notice, because no participant knows both numbers.

Nothing did, anyway.
The replay document now carries the canvas its run was made against:

```cpp
struct ReplayDocument
{
    std::vector<TickEvent> events{};
    std::optional<gfx::Size> canvas{};
};
```

and the reader is given what the caller is about to use, so that one object finally knows both:

```cpp
const ReplayReader reader(std::move(check));
```

## Why it warns instead of refusing

The instinct, having found a mismatch you can detect, is to make it an error.
It is a warning, deliberately, and the reasons are in `CanvasCheck`'s own comment:

> A mismatch is a warning and never an error.
> Refusing the load would break every recording made before the canvas was written into the format, and the caller may well know better than the file does -- so this reports rather than decides.

Both halves matter.
The field is optional in the schema — described, never required — so every replay checked in before it existed still loads, including the demo files under `src/apps/*/replays/`.
And a person deliberately replaying a session at a different size is doing something legitimate: they want to see what happens, and a library has no standing to refuse them.

What it does instead is say exactly what it knows:

```
antwika::replay: this replay was recorded against a 1024x640 canvas
and is being replayed against 800x600; recorded input may land
somewhere else
```

"May land somewhere else" is the honest phrasing.
The library cannot know whether any recorded click was near an edge, so it will not claim the run is wrong — only that this is the condition under which it silently could be.

Three guards come before that message, and each is a different kind of "nothing to say":

```cpp
if (!check.canvas.has_value() || !document.canvas.has_value()) { return; }
if (*check.canvas == *document.canvas) { return; }
if (!check.logger.has_value()) { return; }
```

A document with no canvas predates the field.
A caller with no canvas has claimed none — which is exactly what an app with no pointer input has to say, and why `saveReplayFile()` takes the canvas as an optional rather than requiring every app to invent one.
Either way, the canvas about to be used is the only one anybody knows about, and there is nothing to compare it with.

The logger being optional is the compromise it looks like.
`antwika::replay` owns no logger and has no global to reach for, so a caller with nowhere to report to gets no report.
That is stated in the header rather than hidden, because the alternative — a check important enough to justify a global logger — is how libraries acquire global state.

## The same shape, three more times

Once you are looking for "completes and is wrong", the surrounding code has more of it.

**`--replya demo.json` started an empty session.**
The old parser skipped anything it did not recognise, so a typo in a flag name silently became "no replay was asked for" and the app started a fresh run.
An unknown flag and a flag missing its value both throw `CommandLineError` now, and there is a `--help` listing what is accepted.

**Two parsers refusing each other's flags.**
`apps/poker` takes `--tick-delay-ms`, and the shared replay parser did not know about it.
Parsing twice — once for the replay flags, once for the app's — means each pass sees the other's flags as unknown, so making the first pass strict is what broke the second.
The fix is that there is one pass: `replayCliFlags()` returns a table an app concatenates its own flags onto, and `runRecorded()` parses the result once.
The comment on the field says which bug this is, in five words: *which is how `--tick-delay-ms` stopped working*.

**A missing replay reported as malformed JSON.**
`loadReplayFile()` used to hand an unopened `ifstream` to the reader, which read zero bytes and reported a parse failure.
"Your replay is invalid" sends you to inspect a file that is not there.

```cpp
// A file that is not there is not a malformed one.
```

Two failures, two messages, and the one you get names the thing that actually happened.

**A `--record` run losing everything at the end.**
A recording is written once, when the run ends.
An unwritable path was therefore a whole session discarded in silence, at the only moment it could not be retried.

## What the four have in common

None of them was a logic error.
Every one was a place where two facts had to agree and nothing was holding both: a canvas in a file and a canvas in a program, a flag table and an argv, a filename and a filesystem, a buffer and a disk.

So the rule that came out of it is about *where the check lives*, not about adding more checks.

Where both facts are constants, make it a build error.
`TileAtlas.hpp` asserts its own layout because every number in it is `constexpr`, and its comment says why: *on screen is the only other place it could fail*.
`ui::assertDistinct` does the same for widget ids — two widgets sharing an id is legal and means "these are one widget", so the mistake has a plausible wrong answer and no diagnostic unless a `static_assert` provides one.

Where one fact is only known at runtime, put the two in the same object and make it loud.
`CanvasCheck` exists so that something holds both canvases.
The concatenated flag table exists so that one parse holds every flag.

Where a mismatch is genuinely the caller's business, warn with both values and carry on.
Refusing is a decision, and a library that owns neither the recording nor the intent is not the thing to make it.

The unifying question is not "is this input valid".
It is: **if this is wrong, what tells anybody?**
For each of the four above, the answer used to be "the output looks a bit odd, eventually, to somebody paying attention".
That is not an answer.
