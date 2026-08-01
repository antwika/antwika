# antwika::sound

`src/libs/sound/` — decoding, mixing and playing PCM audio, behind a backend seam.

## What it is for

Turning a WAV into a value, summing several of those into a device's buffer, and handing that buffer to whichever audio framework the build selected.

It owns no thread, no lock, no ring buffer and no atomic.
A device renders when a caller asks it to, through `IDevice::pump()`, and does nothing in between.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `WaveFormat.hpp` | `WaveFormat` | A sample rate and a channel count. |
| `Frames.hpp` | `FrameIndex`, `FrameCount`, `ChannelCount` | The counting vocabulary. |
| `Waveform.hpp` | `Waveform` | Decoded audio as a plain value: interleaved, normalised float. |
| `SampleBuffer.hpp` | `SampleBuffer` | A non-owning **planar** span of per-channel spans. |
| `WavReader.hpp` | `WavReader` | Decodes a `std::istream` into a `Waveform`. |
| `WaveformId.hpp` | `WaveformId` | What a library hands out and a request names. |
| `WaveformLibrary.hpp` | `WaveformLibrary` | Owns every waveform, so nothing on the render path can dangle. |
| `PlayRequest.hpp` | `PlayRequest` | One sound, at an **absolute** start frame, with a gain and a pan. |
| `Mixer.hpp` | `Mixer`, `MixerDesc` | A fixed voice pool that sums voices into a buffer. |
| `IRenderCallback.hpp` | `IRenderCallback` | `render(SampleBuffer, FrameIndex firstFrame) noexcept`. |
| `IDevice.hpp` | `IDevice` | `start`/`stop`/`pump`/`format`/`bufferFrames`/`framesPlayed`. |
| `ISoundBackend.hpp` | `ISoundBackend` | `name`/`capabilities`/`openDevice`. |
| `SoundCapabilities.hpp` | `SoundCapabilities` | Whether it reaches a speaker, and whether it drives itself. |
| `DeviceDesc.hpp` | `DeviceDesc` | What to open a device as. |
| `NullSoundBackend.hpp` | `NullSoundBackend`, `NullDevice` | Plays nothing, in the library rather than under `backends/`. |
| `OfflineDevice.hpp` | `OfflineDevice` | Renders into a `Waveform` instead of at a speaker. |
| `SelectedSoundBackend.hpp` | `makeSelectedSoundBackend()` | Declared here, defined under `backends/`. |
| `SoundError.hpp` | `SoundError` | This library's one failure type. |

## Depends on

[`log`](log.md), and nothing else — not `time`, not `ecs`, not `replay`, not `gfx`.

## Non-obvious decisions

**A device is pumped, not driven, and that is what keeps the project single-threaded.**
The usual arrangement is a callback on the framework's own high-priority thread with a lock-free queue feeding it, which would be a second concurrency model in a codebase that has none.
`pump(frames)` renders exactly that many frames on the calling thread, so a headless run costs no wall-clock time and the whole test suite runs with no hardware.

`SoundCapabilities::selfDriven` keeps the other door open: a backend that genuinely cannot be pumped says so, and the conformance suite skips those tests rather than failing it for being honest.

**A callback is handed the *absolute* index of its first frame.**
Never a count since the last call.
A device that restarts its counter per buffer forces every caller to keep a running total, and every caller that keeps one has somewhere to lose count — after which every scheduled sound is placed at the wrong moment.
`Render_ReceivesAscendingContiguousFrames` in the conformance suite is what catches that, and it is the test in the suite worth the most.

It is also what makes `PlayRequest::startFrame` mean something: a sound placed at frame 48,000 begins there and not at whichever buffer boundary follows. "Play it now" is deliberately not expressible.

**`framesPlayed()` is what has been *consumed*, and it is advisory.**
Legal to read to decide how long to sleep; never to decide what to compute.
A device that consumes instantly reports every frame pumped; a real one lags by whatever is still queued, and pacing needs that lag.

**A waveform is always normalised float.**
There is no sample-format enum anywhere, which deletes an entire conversion matrix: decode once, to a plain value, exactly as [`gfx`](gfx.md) does with a `Bitmap`.
Float is unarguable here because samples never reach simulation state — audio is a write-only projection in exactly rendering's sense.

**Buffers are planar and files are interleaved.**
A mixer wants one contiguous run per channel with no stride arithmetic; a file holds frames. `OfflineDevice` is the one place the two layouts meet.

**A mismatched rate is refused rather than resampled.**
Resampling well is real signal processing and resampling badly is audible, so while there is no resampler the honest answer is to say so.

**The null backend lives in the library rather than under `backends/`.**
That follows `NullInputBackend`, and it is what puts it inside the coverage gate — `backends/*` is exempt and `src/` is not.

**`Mixer::render()` has no error path at all, by construction rather than by care.**
A mixer that exists was built with a valid format and a voice pool sized once, and every voice holds a waveform the library owns, so there is nothing left for the render path to check and nothing for it to allocate.
`WaveformLibrary` owning the waveforms is what makes that true: a mixer holds pointers into it and never a copy, so there is no way to hand it something that could go away underneath it.
`SoundError` is thrown from constructors, from `openDevice()`, from `start()` and from the decoder — never from rendering.

`WavReader` reads from a `std::istream` rather than a path, exactly as `gfx::PngReader` does, and for the same two reasons: the library opens no files, and every refusal it can produce is reachable from bytes in memory.
Every one of them is, which is why the decoder is covered without anything on disk.

## Backend selection

`sound_backend` (Conan) and `ANTWIKA_SOUND_BACKEND` (CMake), with values `null` and `sdl3`.
It defaults to `null` and deliberately offers no `auto`: [`input`](input.md) follows graphics because a window nobody can click is useless, while sound is orthogonal, and following would mean every existing `sdl3` build silently began opening an audio device.

`raylib` is absent from the values because it does not implement this seam, so Conan refuses it before anything is downloaded.

The SDL3 backend claims SDL's audio subsystem and nothing else, so a build selecting it for sound alone never asks for a display — which is why CI runs the sound conformance suite **without** `xvfb-run` while running the graphics and input ones under it.

## The musical layer above this — not built yet

Everything below this heading is **design, not code**.
No type, header or file named here exists; nothing in the tree depends on any of it, and a reader looking for a `Pulse` or an `IPattern` will not find one.
It is recorded here because this library was carved out of a larger plan for a programmable audio player, and the half that shipped is the half described above: decoding, mixing and a device seam.
The half that did not is *musical* — musical time, a pattern algebra, envelopes, a script and plugin hosting — and it is the reason several decisions above were made the way they were.
That plan lived in `docs/audio-player-plan.md` until the work it described had shipped; `docs/` holds only rules that are still normative, so the document went and its unbuilt design came here.

It would arrive as a separate library above this one — `antwika::sound` owns the device and would not gain a note, a bar or a tempo — depending on this library as `antwika::app` depends on [`gfx`](gfx.md).

### The rule the whole thing hangs on

**Information flows simulation to audio and never back.**
A score that schedules is simulation state: the tempo map, the instruments, the patterns, the envelopes and the notes they produce for a span of musical time are pure values from pure functions, reproducible from the tick stream and assertable with `EXPECT_EQ`.
A device that plays is a projection: oscillator phase, envelope stage, filter memory and the samples themselves are regenerable from the note stream and appear in no snapshot, summary or replay.
The single exception is `framesPlayed()`, which the simulation may read to decide how long to *sleep* and never to decide what to *compute* — the standing `simulation::TickPacer`'s wall clock already has.

"Wait until the sound has finished", "trigger the next section when the buffer drains" and "duck the music when the meter is loud" are all the forbidden direction, and each has a legal form in which the simulation computes the condition from musical time it already knows.

### A value model first, a text front-end second

The score is specified as a text language, but the first thing built would be the C++ value model it parses into, with a builder API mirroring the syntax one to one.
That is the [`ui`](ui.md) move: the caller writes immediate-mode code, what it builds is a value, and a whole layout is assertable with no mock.
The payoff is a fully tested pattern algebra with no parser, no grammar, no new dependency and no new error type, and a text front-end afterwards that is a thin translation with exactly one error type of its own.

The illustrative syntax, kept because it is what the value model has to be able to express:

```
# Session-level facts. A replay must reproduce these, so they are events.
sample-rate 48000
ppqn        960
tempo       120 bpm

instrument kick = sampler("kick.wav") {
    gain = -6 db
}

instrument bass = synth(saw) {
    voices        = 4
    filter.cutoff = 400 hz
    filter.q      = 0.7
    amp.adsr      = adsr(5 ms, 120 ms, 0.6, 200 ms)
}

# A pattern is a function from a span of musical time to note events.
pattern four   = euclid(4, 16)
pattern clave  = euclid(3, 8) |> rotate(2)
pattern arp    = notes(C2, Eb2, G2, Bb2)

# `*` gates one pattern by another; `+` stacks; `,` concatenates one
# cycle each.
pattern riff   = arp * euclid(5, 16)
pattern fill   = four + (clave |> transpose(+12))
pattern verse  = (four, four, four, fill)

pattern lead   = riff
                 |> every(4, retrograde)
                 |> stretch(3/2)
                 |> degrade(0.2, seed 7)
                 |> scale(minor-pentatonic, root C)

# An envelope is a unit-valued function of its own phase, in musical time.
envelope sweep   = ramp(0 -> 1) over 8 bars then hold
envelope breathe = adsr(1 bar, 2 bars, 0.4, 4 bars) |> loop(8 bars)
envelope pluck   = curve(exponential, 1 -> 0) over 1/4 note

track drums = kick <- four
track hats  = hihat <- clave
track low   = bass <- riff
track top   = bass <- lead

# Any parameter, not just amplitude. `density` is a pattern parameter and
# `cutoff` is an instrument parameter, and they modulate identically.
modulate low.filter.cutoff by sweep   depth 3000 hz
modulate hats.density      by breathe depth 0.5
modulate top.pan           by breathe depth 0.8 offset -0.4

arrange {
    bar  0 ..  8 : drums
    bar  4 .. 16 : drums, low
    bar 16 .. 32 : drums, low, top
    bar 32       : stop
}
```

Two things in that snippet are load-bearing rather than decorative, and they are the two rules most easily lost.

**`degrade(0.2, seed 7)` is positional randomness, never a stateful generator.**
It is `hash(seed, patternId, pulse) < threshold`, so the answer for a pulse is the same whether it was asked first or four hundred bars in.
A stateful generator would make a pattern's output depend on the order and number of queries, which breaks both the "ask about bar 400 directly" property and replay determinism the moment a lookahead window changes size.
[`rng`](rng.md) already exists and its output sequence is already part of its contract, which is most of what this needs.

**`stretch(3/2)` takes a rational, never a float.**
Every time transformation is an exact integer-ratio map on pulses, so composing four of them still lands on an exact pulse and never accumulates drift.
A pattern that cannot express its stretch as a ratio is one the library refuses.

### Musical time

- `Pulse` — a `std::uint64_t` count of PPQN subdivisions from session start; 960 PPQN divides cleanly by 2, 3, 4, 5 and 6, which covers triplets and quintuplets without rounding.
- `PulseSpan` — a half-open `[begin, end)` window; empty and reversed throw `ScoreError` rather than being tolerated.
- `Ratio` — an exact reduced `{numerator, denominator}` of `std::int64_t`, used for tempo, stretch and every other time scaling.
- `TempoMap` — a sorted table of segments, each holding its start pulse, its start frame and its tempo as a `Ratio`; a lookup is a binary search plus one exact multiply, so `framesAt(Pulse)` and `pulseAt(FrameIndex)` are exact inverses at segment boundaries and monotone everywhere.

`FrameIndex` is the one true clock of the whole system, and it already exists here in `Frames.hpp`.

**`Tick` to `Pulse` is therefore a composition of two exact integer maps and never a floating-point division**, which is the answer to "the sample rate does not divide evenly into the tick rate": it does not need to.
Frames per tick is held as a `Ratio` and `framesAtTick(t)` is `floor(t * numerator / denominator)`, so the residue is carried in the expression rather than in a running variable.
48000 over 60 is 800 exactly; 48000 over 59 alternates 813 and 814 forever without drifting.

A tick does not *contain* musical time, it *schedules* it: the tick loop is when decisions are made, and musical time is what those decisions are expressed in.

### Notes, parameters and patterns

- `Pitch` — a semitone integer with middle C at 60, and `Velocity` — a `std::uint8_t` from 0 to 127, both to match every other tool on earth.
- `NoteEvent` — `{ Pulse start; Pulse duration; Pitch pitch; Velocity velocity; VoiceTag tag; }`, trivially copyable and comparable, so a whole span of a score is one `EXPECT_EQ`.
- `ParamId` — symbolic and caller-supplied exactly as `ui::WidgetId` is, and for the same reason: an id is what crosses between subsystems, so it must not be declaration order.
- `ParamValue` — fixed point rather than float, because a parameter's value is simulation state.

`IPattern` offers `notesIn(PulseSpan window, IPatternSink &out) const` and `period()`.
**A window rather than a "next event" cursor is the decision the rest depends on.**
A cursor is stateful, so it cannot be shared between a sequencer and a test, cannot be queried out of order, and cannot be transformed without the transformation becoming stateful too.
A window composes instead: `transpose` forwards it unchanged and rewrites pitches, `stretch(3/2)` maps it backwards through the inverse ratio before forwarding, and `retrograde` reflects it within the enclosing cycle.
Each combinator is a small class holding a reference to its operand, and the algebra is closed under composition with no special cases.
An `IPatternSink` rather than a returned vector leaves allocation at the caller's discretion, which is what lets a sequencer emit into a pre-sized scratch buffer.

The first combinator set: `euclid(k, n)`, `steps("x..x")`, `notes(...)`, `rest(n)`, `rotate(n)`, `transpose(n)`, `retrograde()`, `invert(axis)`, `stretch(Ratio)`, `repeat(n)`, `every(n, f)`, `stack(a, b)`, `concat(a, b)`, `gate(a, b)`, `degrade(p, seed)`, `scale(intervals, root)`.

### Envelopes and modulation

Two distinct things share the word, and conflating them is the trap.

`IEnvelope` lives in **musical time** and is a pure `ParamValue unitAt(Pulse phase) const`; it is simulation state, evaluated on the simulation thread, and it is what `modulate ... by ...` refers to.
An `Adsr` lives in **frame time** and is a plain value of four frame counts plus a sustain level, evaluated inside a voice by a branch-free state machine; it is projection state and never appears in a score comparison.

A `Modulation` is `{ ParamId target; const IEnvelope &source; ParamValue depth; ParamValue offset; }` and a `ModulationMatrix` is a flat vector of them evaluated once per tick.
Because the target is only a `ParamId`, modulating a filter cutoff, a pan position and a pattern's density are one code path — which is the generality the whole idea rests on.
Pattern density is not special: a density-gated pattern reads its threshold from a `ParamId`, and the matrix writes that id like any other.

### Errors it would add

`ScoreError` for a malformed score — an empty or reversed `PulseSpan`, a `Ratio` with a zero denominator, a `ParamId` nothing declares, a modulation whose depth leaves a parameter's range.
`ScriptParseError` for the text front-end, and `PluginError` for hosting, each arriving with the thing that needs it and not before.
`SoundError` above already covers what the plan called `AudioError`.

### Plugin hosting

**CLAP is the right first choice and VST3 is not.**
CLAP is MIT-licensed and a pure C header with no runtime to link, so it drops into Conan and `backends/` with no licensing question.
Its event model is a flat list of headers each carrying a sample offset in ascending order, which is the design above with a different struct name, so an adapter is a translation rather than an architecture.
Its threading contract is explicit — every call annotated `[main-thread]` or `[audio-thread]` — so this repository's rules and CLAP's are the same rules.
Automation is sample-accurate through the same event list, and extensions are opt-in, so a minimal headless host really is minimal.

VST3's SDK is dual-licensed GPLv3 or a signed Steinberg agreement, and neither suits a statically linked tree; AU is macOS-only and LV2's RDF metadata story buys nothing here.
**VST3 is obtained through `clap-wrapper`**, which is MIT-licensed and produces a VST3 or AU binary from a CLAP one — so CLAP is not a bet against VST3, it is the cheapest path to it.

Hosting sits behind its own seam and deliberately **not** the device seam: a device backend is chosen at build time because a process links one framework, while a plugin is a dynamic library discovered at run time and several are loaded at once.

**A hosted plugin is projection-side only, and there must be no API through which it could inform the score.**
A third-party plugin is opaque, may hold internal randomness and may not be bit-reproducible even with itself.
So a replay of a session with plugins reproduces the note stream exactly and the samples only approximately, which is worth stating rather than quietly breaking.

Running Antwika *as* a plugin inverts the loop, and `EngineLoop`'s shape survives it: a plugin build supplies an `ITickEventSource` fed from the host's event list, derives its tick from the transport's beat position, and steps the engine as many ticks as the buffer spans.
Three things need answering first — a host may rewind or loop the transport where a tick source is asked for each tick once in increasing order, a host may call `process()` on any thread with a changing buffer size, and plugin state save/restore is whole-score serialisation.

### What determinism would then promise

The promise is layered, and being explicit about the layers is what keeps it honest.

- **The note stream is bit-exact, always** — for a given score, seed, tempo map and tick stream, across compilers, platforms, backends and runs, because it is integers and fixed point through pure functions.
- **The command stream is bit-exact, always** — which frame a note lands on comes from exact integer maps.
- **Antwika's own rendered samples are bit-exact for a fixed build**, and only practically identical across compilers, since IEEE-754 leaves latitude in transcendentals and in expression contraction.
- **Rendered samples with third-party plugins are not promised at all.**

A test needing a number asserts on the note stream or a hash of it, and reserves exact-buffer comparison for built-in voices in a pinned offline build.

### The real-time machinery that is deferred rather than pending

The plan assumed a framework-owned callback thread and designed a single-producer single-consumer lock-free ring, a lookahead sequencer running some ticks ahead of the audio clock, an overflow counter read by the simulation thread, a denormal guard and a `scripts/check_audio_thread_purity.py` enforcing "no allocation, no lock, no exception, no logging, no I/O and no unbounded work on the audio thread".
The pumped device model removed the reason for all of it: with no thread of ours there is nothing for a ring to cross.
It becomes live again only if a backend arrives that cannot be pumped, which is what `SoundCapabilities::selfDriven` exists to let one say.

What is still missing before any of this plays *from a tick loop* is the sequencer, and the sequencer needs musical time — so the remaining work is upstream of the device rather than at it.

### Open questions

- **How wide should a `ParamValue` be?** A single fixed-point width is simple and may not hold a cutoff in hertz well; the alternative is a per-parameter unit and range, which is more honest and more code.
- **May the musical layer depend on [`ecs`](ecs.md)?** The recommendation is no, mirroring [`ui`](ui.md), with a sequencer sink living in the application — but a voice pool is a very natural `World`, and that argument deserves a hearing.
- **How is a live MIDI keyboard recorded?** MIDI input is *input* and belongs behind an [`input`](input.md)-shaped seam so it arrives as edges in the tick stream, but that quantises a note to a tick, which a performer notices.
- **What is the right tick rate for music?** Around 17 ms of granularity is fine for scheduling ahead and coarse for live input; a higher tick rate is legal but makes an app's replays incomparable with the other apps'.
- **Should a score be serialisable independently of the script?** A plugin build needs it for state, and a JSON score would let a replay carry a score inline rather than by path.
- **How is a long file streamed from disk?** `WavReader` decodes a `Waveform` whole, which does not fit streaming; nothing needs it yet.

## See also

- [sound_demo](../apps/sound_demo.md) — the showcase.
