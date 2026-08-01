# Plan: a programmable audio player, and an `antwika::audio` library

A plan for playing sound and music from a script: instruments and tracks set up in a small language, notes described by *programmable patterns* rather than literal note lists, envelopes modulating arbitrary parameters, and a real audio device behind a build-time backend seam that no code under `src/` ever names.

**Status: the PCM half has shipped as `antwika::sound`; the musical half has not started.**
Phase 3, most of Phase 4 and the surviving part of Phase 5 are real code now, described by [`wiki/libraries/sound.md`](../wiki/libraries/sound.md) rather than by this plan, and the phase list below says which parts of each landed.
Sound is audible today, through `backends/sdl3` and `apps/sound_demo`; what it is not yet is *musical*.
What remains unbuilt is everything this document is actually about: musical time, the pattern algebra, envelopes, the script and the plugin work.
`antwika::audio` is therefore redefined as the **musical layer above `antwika::sound`**, depending on it as `antwika::app` depends on `antwika::gfx`, rather than as the library that also owns the device.

This document is a design, not a record.
It is opinionated on purpose — where there is a choice, it makes one and says why, so that a reader disagreeing knows exactly what to argue with.

## Goals

- A **script** sets up instruments and tracks, and describes what they play.
- A pattern is a **pure function of musical time**, not a stored array of notes, so it can be transformed, composed, and asked about bar 400 without simulating bars 0 to 399.
- Pattern algebra is a first-class part of the library: Euclidean rhythms, transposition, retrograde, inversion, rational time-stretching, probability, concatenation and stacking, all closed under composition.
- **Envelopes modulate any parameter**, not just amplitude: filter cutoff, gain, pan, and pattern density are the same mechanism with a different target.
- The whole score is **deterministic and replayable** under the existing `antwika::engine` tick loop, and every part of it is testable with no sound card, no thread and no device.
- The concrete audio framework lives under `backends/`, chosen at build time by `ANTWIKA_AUDIO_BACKEND`, exactly as `antwika::gfx` and `antwika::input` do today; the default is `null` and costs the default build no new dependency.
- One exception type per failure category, no global state, `I`-prefixed injected interfaces.

## Non-goals

- **Not a DAW.** There is no timeline editor, no undo stack, no arbitrary audio-file arrangement, and no mixdown session format.
- **Not bit-exact audio across compilers.** Determinism is claimed for the *note stream* and refused for the *sample stream* under third-party plugins; see [Determinism, precisely scoped](#determinism-precisely-scoped) for what is and is not promised.
- **Not low-latency live performance** in the first phases; the design admits latency openly and buys determinism with it.
- **Not a general DSP framework.** A small, fixed set of built-in voices exists to make the sequencer audible; everything richer is a hosted plugin's job.
- **No MIDI hardware** in the first phases, and when it arrives it arrives through the *input* seam and never through the audio seam.

## Where this sits in the existing architecture

Three of this repository's standing decisions constrain the design before anything else does.

**The fixed tick loop with deterministic replay is the core value.**
`simulation::EngineLoop` asks an `ITickSource` for a tick's events, dispatches them, and steps the engine; live and replayed runs differ only in what implements the source.
Any audio design that lets the sound device's clock influence what the simulation computes destroys that, and no amount of care elsewhere buys it back.

**Rendering is a write-only projection of state that can never feed back into the tick loop.**
`blog/012-a-window-that-cant-talk-back.md` sets that rule for graphics.
The interesting question for audio is whether it is the same kind of thing, and the answer is a qualified yes, argued in the next section.

**A framework never appears in `src/`.**
`gfx::makeSelectedBackend()` is declared in `src/` and defined under `backends/`, and linking without a backend is a link error by design.
`antwika::audio` copies that structure line for line rather than inventing a second one.

## Is audio a write-only projection?

Split the system in two and the answer becomes obvious.

**A script that schedules is state.**
The tempo map, the instrument definitions, the patterns, the envelopes, the modulation routings, and the note events those produce for a given span of musical time are all pure values computed by pure functions.
They are simulation state in exactly the sense `game`'s camera is: reproducible from the tick stream, asserted with `EXPECT_EQ`, and never dependent on a device.

**A DAC that plays is not.**
Oscillator phase, envelope stage, filter memory, the voice-stealing choice a polyphony limit forced, and the samples themselves are a *projection* of that state.
They are regenerable from the note stream the way `game`'s road-tile mask is regenerable from the paths — and, like that mask, they deliberately do not appear in a snapshot, a summary, or a replay.

So audio is the same kind of thing as graphics, and differs in three ways that the design has to handle rather than wish away.

- **The projection is stateful and continuous.** A frame is drawn and forgotten; a sample stream must be produced without gaps forever, so the audio side carries state between callbacks.
  That state is *derived* state, and the rule is that nothing may read it back into the simulation.
- **The projection runs on another thread at another rate.** `RenderSystem` runs inside the tick; an audio callback does not.
  The seam is therefore a queue rather than a call, and the queue is the hard part of this plan.
- **The temptation to feed back is much stronger.** "Wait until the sound has finished", "trigger the next section when the buffer drains", "duck the music when the meter is loud", and "compensate for the plugin's reported latency" are all natural requests, and all of them are the forbidden direction.
  Every one of them has a legal form in which the *simulation* computes the condition from musical time it already knows, and the audio thread merely obeys.

The rule, stated once: **information flows simulation to audio and never back.**
The single exception is a monotonic sample counter the audio thread publishes, and the simulation may use it *only* to decide how long to sleep — never to decide what to compute.
That is the same standing that wall-clock time already has in `simulation::TickPacer`, which "touches neither `World` nor the tick it is given", and it is legal for the same reason: pacing changes how long a run takes, never what it computes.

## The script and DSL model

### The decision: a value model first, a text front-end second

The script is specified as a text language, but the **first thing built is the C++ value model it parses into**, with a builder API that mirrors the syntax one to one.

This is the `antwika::ui` move.
There, the caller writes immediate-mode code, what it builds is a flat arena, and `finish()` returns a `Frame` — "a picture is a value", which is what lets a whole layout be asserted with `EXPECT_EQ` and no mock.
Here, a *score* is a value, and a text file is one way to obtain one.

The payoff is that Phase 1 ships a fully tested pattern algebra with no parser, no grammar, no new dependency and no new error type, and the text front-end in a later phase is then a thin, separately testable translation with exactly one new error type of its own.
The cost is that the syntax below is not runnable until that later phase, which is acceptable because the syntax is the easy part.

### Illustrative syntax

The language is declarative, whitespace-tolerant, and has no control flow.
Everything it can express is a value, and every value is a pure function of musical time.

```
# Session-level facts. A replay must reproduce these, so they are events.
sample-rate 48000
ppqn        960
tempo       120 bpm

# --- instruments -------------------------------------------------------
instrument kick = sampler("kick.wav") {
    gain = -6 db
}

instrument bass = synth(saw) {
    voices        = 4
    filter.cutoff = 400 hz
    filter.q      = 0.7
    amp.adsr      = adsr(5 ms, 120 ms, 0.6, 200 ms)
}

# --- patterns ----------------------------------------------------------
# A pattern is a function from a span of musical time to note events.
pattern four   = euclid(4, 16)
pattern clave  = euclid(3, 8) |> rotate(2)
pattern arp    = notes(C2, Eb2, G2, Bb2)

# Composition. `*` gates one pattern by another; `+` stacks; `,` concatenates
# one cycle each.
pattern riff   = arp * euclid(5, 16)
pattern fill   = four + (clave |> transpose(+12))
pattern verse  = (four, four, four, fill)

# Transformations are ordinary functions, so they compose.
pattern lead   = riff
                 |> every(4, retrograde)
                 |> stretch(3/2)
                 |> degrade(0.2, seed 7)
                 |> scale(minor-pentatonic, root C)

# --- envelopes ---------------------------------------------------------
# An envelope is a unit-valued function of its own phase, in musical time.
envelope sweep   = ramp(0 -> 1) over 8 bars then hold
envelope breathe = adsr(1 bar, 2 bars, 0.4, 4 bars) |> loop(8 bars)
envelope pluck   = curve(exponential, 1 -> 0) over 1/4 note

# --- tracks and routing ------------------------------------------------
track drums = kick <- four
track hats  = hihat <- clave
track low   = bass <- riff
track top   = bass <- lead

# Any parameter, not just amplitude. `density` is a pattern parameter and
# `cutoff` is an instrument parameter, and they modulate identically.
modulate low.filter.cutoff by sweep   depth 3000 hz
modulate hats.density      by breathe depth 0.5
modulate top.pan           by breathe depth 0.8 offset -0.4

# --- arrangement -------------------------------------------------------
arrange {
    bar  0 ..  8 : drums
    bar  4 .. 16 : drums, low
    bar 16 .. 32 : drums, low, top
    bar 32       : stop
}
```

Two things in that snippet are load-bearing rather than decorative.

`degrade(0.2, seed 7)` is **positional** randomness, not a stateful generator.
It is defined as `hash(seed, patternId, pulse) < threshold`, so the answer for a given pulse is the same whether you asked about it first or four hundred bars in.
A stateful RNG would make a pattern's output depend on the order and number of queries, which destroys both the "ask about bar 400 directly" property and replay determinism the moment a lookahead window changes size.
This is the single most important rule in the pattern algebra, and it is why `antwika::rng` with a positional hash is a prerequisite rather than a nicety.

`stretch(3/2)` takes a **rational**, not a float.
Every time transformation is an exact integer-ratio map on pulses, so composing four of them still lands on an exact pulse and never accumulates drift.
A pattern that cannot express its stretch as a ratio is a pattern this library refuses.

## Core value types and interfaces

### Musical time

- `Pulse` — `std::uint64_t`, a count of PPQN subdivisions from session start; 960 PPQN divides by 2, 3, 4, 5 and 6 cleanly, which covers triplets and quintuplets without rounding.
- `PulseSpan` — a half-open `[begin, end)` window; the empty and reversed cases throw `ScoreError` rather than being silently tolerated.
- `SampleIndex` — `std::uint64_t`, a count of frames from session start, and **the one true clock of the whole system**.
- `Ratio` — an exact `{numerator, denominator}` of `std::int64_t`, always reduced, used for tempo, stretch and every other time scaling.
- `TempoMap` — a sorted table of segments, each recording the pulse it starts at, the sample it starts at, and its tempo as a `Ratio`.
  Lookup is a binary search plus one exact integer multiply, so `samplesAt(Pulse)` and `pulseAt(SampleIndex)` are exact inverses at segment boundaries and monotone everywhere.

### Notes and parameters

- `Pitch` — a semitone integer, middle C fixed at 60 to match every other tool on earth.
- `Velocity` — `std::uint8_t`, 0 to 127, for the same reason.
- `NoteEvent` — `{ Pulse start; Pulse duration; Pitch pitch; Velocity velocity; VoiceTag tag; }`, trivially copyable, comparable, and hashable, so a whole span of a score can be compared with one `EXPECT_EQ`.
- `ParamId` — a symbolic, caller-supplied identifier, exactly as `ui::WidgetId` is, and for exactly the same reason: an id is what crosses between subsystems, so it must not be declaration order.
- `ParamValue` — a `Q16.16` fixed-point scalar in a declared range, because a parameter's *value* is simulation state and therefore must not be a float.

### Patterns

```
class IPattern
{
public:
    virtual ~IPattern() = default;

    // Emit every note that starts within `window`, in ascending start
    // order, into `out`. Pure: two calls with the same window emit the
    // same events, in the same order, forever.
    virtual void notesIn(PulseSpan window, IPatternSink &out) const = 0;

    // The pattern's repeat length, if it has one. Combinators use this to
    // compute their own; `nullopt` means aperiodic.
    [[nodiscard]] virtual std::optional<Pulse> period() const = 0;
};
```

A **window** rather than a "next event" cursor is the decision that makes everything else work.
A cursor is stateful, so it cannot be shared between the sequencer and a test, cannot be queried out of order, and cannot be transformed without the transformation also becoming stateful.
A window composes: `transpose` forwards the window unchanged and rewrites pitches, `stretch(3/2)` maps the window backwards through the inverse ratio before forwarding it, and `retrograde` reflects it within the enclosing cycle.
Each combinator is a small class holding a reference to its operand, and the whole algebra is closed under composition with no special cases.

An `IPatternSink` rather than a returned `std::vector` keeps allocation at the caller's discretion, which matters because the sequencer wants to emit straight into a pre-sized scratch buffer.

The combinator set for Phase 1: `euclid(k, n)`, `steps("x..x")`, `notes(...)`, `rest(n)`, `rotate(n)`, `transpose(n)`, `retrograde()`, `invert(axis)`, `stretch(Ratio)`, `repeat(n)`, `every(n, f)`, `stack(a, b)`, `concat(a, b)`, `gate(a, b)`, `degrade(p, seed)`, `scale(intervals, root)`.

### Envelopes and modulation

Two distinct things share the word "envelope", and conflating them is the trap.

- `IEnvelope` lives in **musical time** and is a pure function `ParamValue unitAt(Pulse phase) const`.
   It is simulation state, it is evaluated on the simulation thread, and it is what `modulate ... by ...` refers to.
  `Ramp`, `Curve`, `LoopedEnvelope` and a musical-time `AdsrEnvelope` implement it.
- `Adsr` lives in **sample time** and is a plain value struct of four frame counts plus a sustain level, evaluated on the audio thread by a branch-free state machine inside a voice.
  It is projection state, it allocates nothing, and it never appears in a score comparison.

A `Modulation` is `{ ParamId target; const IEnvelope &source; ParamValue depth; ParamValue offset; }`, and a `ModulationMatrix` is a flat vector of them evaluated once per tick.
Because the target is just a `ParamId`, modulating a filter cutoff, a pan position and a pattern's density are the same code path — which is precisely the generality the requirement asked for.
Pattern density is not special: a `DensityGated` pattern reads its threshold from a `ParamId`, and the matrix writes that id like any other.

### The device seam

**This section has been built, and the real headers are the authority.**
It lives in `antwika::sound` rather than `antwika::audio`, under `src/libs/sound/include/antwika/sound/`: `IRenderCallback`, `IDevice`, `ISoundBackend`, `SoundCapabilities`, `SoundError`, `DeviceDesc`, `SampleBuffer` and `makeSelectedSoundBackend()`.
Read [`wiki/libraries/sound.md`](../wiki/libraries/sound.md) for why it is shaped the way it is; what follows is only where the built seam departs from what this plan first sketched, and why.

**A device is *pumped* rather than driven, and this is the biggest departure.**
`IDevice::pump(frames)` renders exactly that many frames on the calling thread, so there is no audio thread, no lock and no ring buffer anywhere in the library.
The plan below assumed a framework-owned callback thread and designed an SPSC ring to feed it; that turned out to be avoidable, because SDL3's `SDL_OpenAudioDeviceStream` offers a push model where the caller renders and hands buffers over.
`SoundCapabilities::selfDriven` is how a backend that genuinely cannot be pumped says so, which keeps the callback model reachable without committing to it.
Phase 5's ring, lookahead sequencer and thread-purity checker are therefore **deferred rather than pending**, and the "is a second thread acceptable" open question below is not yet live.

**A callback is handed an absolute frame index**, exactly as this plan wanted, and that is the one part worth restating rather than relaxing.
`framesPlayed()` kept its monotonic-and-advisory contract verbatim, stated in its own doc block where it cannot be forgotten.

Three names changed and one type did.
`IAudioCallback` is `IRenderCallback`, `AudioBuffer` is `SampleBuffer`, and `IAudioDevice::sampleRate()` is `IDevice::format()` returning a `WaveFormat`, since a rate without a channel count describes nothing a buffer could be written to.
`SampleBuffer` is planar and non-owning as designed, so nothing on the render path allocates.

Floating-point samples are used for the *buffer* because every framework wants them and because the sample values are projection state, where bit-exactness is not promised anyway.
Every value that is simulation state — pitch, velocity, parameter value, time — stays integer or fixed point.
That line is the whole determinism story in one sentence.

**The `null` backend** reports one device at any requested rate, never starts a thread, and discards every sample.
It lives *in* the library rather than under `backends/`, following `NullInputBackend`, which is what puts it inside the coverage gate.
Critically, it does not advance its own clock: it exposes `pump(FrameCount)`, and a headless run advances it from the tick loop.
So a `null` audio run is instantaneous, reproducible, and produces exactly the same note stream as a real one — the same standing `gfx::NullBackend` has today.

**An `OfflineDevice`** is the second thing built and the workhorse of every test: it renders a requested number of frames into a caller-owned buffer, synchronously, on the calling thread, as fast as the CPU allows.
It is not a backend; it lives in `src/libs/audio/` because it names no framework.

### Errors

One type per failure category, following the existing rule.

- `AudioError` — a device or backend could not honour a request: no device, an unsupported rate, a failed start.
- `ScoreError` — a score is malformed: an empty or reversed `PulseSpan`, a `Ratio` with a zero denominator, a `ParamId` nothing declares, a modulation whose depth leaves a parameter's range.
- `ScriptParseError` — the text front-end could not parse or resolve a script; arrives with the front-end in Phase 6, not before.
- `PluginError` — a hosted plugin could not be loaded, instantiated or activated; arrives with hosting in Phase 7, not before.

## Timing and threading

### How musical time relates to `Tick`

`SampleIndex` is the one true clock, and both other clocks map into it by exact integer arithmetic.

**Ticks to samples.**
The sample rate does not have to divide evenly into the tick rate, and the design does not require it to.
`samplesPerTick` is held as a `Ratio` (`sampleRate / tickRate`), and `samplesAtTick(t)` is `floor(t * numerator / denominator)` evaluated in 128-bit-safe integer arithmetic.
That is exact, monotone, and free of accumulated drift, because the residue is carried in the expression rather than in a running variable.
At the convenient rates it degenerates to the obvious answer — 48000 over 60 is 800 samples per tick exactly — and at inconvenient ones such as 44100 over 60 it alternates 735 and 735 with no error at all, while 48000 over 59 alternates 813 and 814 forever without drifting.

**Pulses to samples.**
`TempoMap` does the same job with the same machinery: each segment stores its start pulse, its start sample, and its samples-per-pulse `Ratio`, and a lookup is a binary search plus one exact multiply.
A tempo change is an *event* on a tick, so it appends a segment whose start sample is computed from the previous segment, and a replay reconstructs the identical table.

**Therefore `Tick` to `Pulse` is a composition of two exact integer maps** and never a floating-point division.
This is the direct answer to "the sample rate does not divide evenly into ticks": it does not need to, because neither map rounds, and the only rounding in the system is the single `floor` at the end of each, which is deterministic.

A tick does not *contain* musical time; a tick *schedules* it.
The relationship is one of authority, not of containment: the tick loop is when decisions are made, and musical time is what those decisions are expressed in.

### The seam

```
  simulation thread                        audio thread
  ------------------                       ------------
  EngineLoop
    dispatch tick events
    SequencerSink::onTick(t)
      window = [ pulseAt(sample(t+L)),
                 pulseAt(sample(t+L+1)) )
      score.notesIn(window, scratch)
      modulation.evaluate(window.begin)
      for each event:                       IAudioDevice::render(out, f0)
        ring.push(AudioCommand{...})   -->    while ring.peek().frame < f0+n:
                                                ring.pop() -> apply to voice
                                              for each active voice:
                                                voice.render(out, ...)
                                              mixer.finish(out)
    engine.step(t)
```

The sequencer runs **L ticks ahead** of the audio clock, where L defaults to three ticks and has a documented minimum of one audio buffer plus one tick.
That is the standard answer to scheduling against a clock you do not own, and it buys the whole design its determinism: the simulation never waits on the device, and the device never waits on the simulation.
The price is latency of roughly `L` ticks, about 50 ms at 60 Hz, and this plan pays it without complaint, because the alternative is letting the audio callback drive the simulation.

The queue is a **single-producer single-consumer lock-free ring** of trivially-copyable `AudioCommand` values, each carrying an absolute `SampleIndex` and a small tagged union of note-on, note-off, parameter-set and all-notes-off.
Absolute frame timestamps rather than offsets, so a command is meaningful independently of which callback happens to consume it, and so sample-accurate placement inside a buffer falls out for free.

Backpressure is the producer's problem and must never block: a full ring increments an overflow counter that the **simulation thread** reads and logs on its next tick.
Logging from the audio thread is forbidden, and this is the pattern that makes that survivable.

Voices come from a fixed, pre-allocated pool sized at device open.
Exceeding polyphony steals the oldest voice by a deterministic rule, and — importantly — the *decision* about which voice to steal is made on the audio thread, which is legal precisely because it is projection state that nothing reads back.

### What must never happen on the audio thread

Every one of these can block for an unbounded time, and a blocked audio callback is an audible glitch.

- **Allocation or deallocation** of any kind: `new`, `delete`, `malloc`, `std::vector` growth, `std::string`, `std::function` with a captured heap payload, and any `shared_ptr` copy whose refcount might drop to zero.
- **Any lock**: `std::mutex`, `std::condition_variable`, and equally any lock hidden inside a library call.
  A priority-inverted audio thread waiting on a lower-priority simulation thread is the classic way to produce a dropout.
- **Exceptions**: `IAudioCallback::render` is `noexcept`, so throwing terminates.
  Nothing on the audio path may throw, which means nothing on it may allocate, which is the same rule twice.
- **Logging**: `antwika::log` formats and appends, and both allocate.
  Diagnostics from the audio thread are counters read by the simulation thread.
- **File or network I/O**, and any syscall that can sleep.
  Samples are loaded, decoded and uploaded before a voice can reference them, exactly as `gfx::PngReader` decodes before `createTexture()` uploads.
- **Unbounded work**: no `while` loop whose trip count is not a function of the buffer size and the polyphony limit.
- **Denormals**: floating-point denormals are not incorrect, they are slow enough to blow a deadline, so the callback sets flush-to-zero and denormals-are-zero on entry and restores on exit.

These rules are enforceable mechanically, and this repository already has the habit: alongside `check_line_length.py` and friends, a `scripts/check_audio_thread_purity.py` greps the translation units marked as audio-thread code for the forbidden constructs and fails CI on a hit.
A comment convention marks the boundary, so the check has something unambiguous to scope itself to.

## VST and CLAP compatibility

Two directions, and they deserve separate answers.

### Hosting third-party plugins

**VST3** is the format with the market, and it is the wrong first choice here.
Its SDK is dual-licensed under GPLv3 or a proprietary Steinberg agreement, and neither suits this repository: GPLv3 is viral across a statically linked C++ tree, and the proprietary route requires a signed agreement and registration.
Technically it is a COM-like C++ ABI with `FUnknown` reference counting, a factory, and a deliberate split between `IComponent`/`IAudioProcessor` and `IEditController` that a headless host must implement both halves of regardless.
The API surface is large, the documentation is generated, and none of it maps cleanly onto a small backend seam.

**CLAP** is the right first choice, and possibly the only one.

- It is **MIT-licensed and a pure C header** with no runtime and no library to link, so it drops into Conan and into `backends/` with no licensing question at all.
- Its **event model is isomorphic to the design above**: `process()` receives a flat list of `clap_event_header` values, each with a sample-offset timestamp, in ascending order.
  That is the `AudioCommand` ring with a different struct name, so the adapter is a translation and not an architecture.
- Its **threading contract is explicit**: every call in every extension is annotated `[main-thread]` or `[audio-thread]`, and the specification states plainly what may not happen on the latter.
  This repository's rules and CLAP's rules are the same rules, which means a conformant host is not fighting the format.
- **Automation is sample-accurate** through the same event list, so a modulation envelope evaluated per tick lands on the exact frame it should rather than being quantised to a buffer boundary.
- Extensions are opt-in, so a minimal host — instantiate, activate, process notes, set parameters — is genuinely minimal, and a GUI is a later extension rather than a mandatory half of the API.

**AU** is macOS-only and **LV2** carries an RDF/Turtle metadata story that buys nothing here; both are declined.

**Recommendation: host CLAP, and do not host VST3.**
If VST3 hosting is ever genuinely required, the honest route is a separate, optionally compiled module under a proprietary-license flag, kept entirely out of the default build — not a compromise threaded through the core.

Hosting sits behind its own seam, and deliberately **not** the device seam.
A device backend is selected at build time because a process links one framework; a plugin is a dynamic library discovered at *run* time from a path, and several are loaded at once.
So `IPluginHost` / `IPluginInstance` live in `antwika::audio`, the CLAP implementation lives under `backends/clap/`, and it is enabled by a Conan option `-o audio_plugins=clap`, defaulting to `none` so the default build gains no dependency — the same shape `gfx_backend=null` has today.

**A hosted plugin is projection-side only, and the document must say so loudly.**
A third-party plugin is opaque, may hold internal randomness, may behave differently on different hardware, and may not be bit-reproducible even with itself.
Therefore a plugin may colour the sound and may never inform the score: nothing a plugin returns crosses back into an `ITickSource`, an `IEventSink`, or any parameter the score reads.
A replay of a session with plugins reproduces the note stream exactly and the samples only approximately, and that is a promise worth stating rather than one worth quietly breaking.

### Exposing Antwika as a plugin

This inverts the loop, and it is the interesting part.

As a plugin, the *host* owns the clock: it calls `process()` with a buffer, a transport position and an event list, and the plugin must produce that buffer and return.
`EngineLoop` cannot drive itself in that world — but its shape survives intact, because the loop is already "ask a source for a tick's events, dispatch them, step the engine".
A plugin build supplies an `ITickSource` fed from the host's event list, derives its tick from the host transport's beat position, and steps the engine as many ticks as the incoming buffer spans.
Everything downstream is unchanged, which is a strong endorsement of the existing architecture.

Three things need real thought before it is attempted.

- **The host may rewind, loop, or jump the transport**, and `ITickSource` documents that ticks are asked for once each in increasing order.
  Honouring a loop means rebuilding state from tick zero or from a checkpoint, and checkpointing is a feature the repository does not have.
- **The host may run `process()` on any thread and may change buffer size**, so everything the tick loop currently does at leisure now happens under audio-thread rules.
- **Plugin state save and restore** is a serialisation problem for the whole score, which the text front-end mostly solves as a side effect.

**Recommendation: build a CLAP plugin, and obtain VST3 through `clap-wrapper`.**
`clap-wrapper` is MIT-licensed and produces a VST3 (and AU) binary from a CLAP plugin, which means VST3 *distribution* is available without ever compiling against the Steinberg SDK or accepting its licence.
That single fact resolves the whole VST-versus-CLAP question: CLAP is not a bet against VST3, it is the cheapest path to it.

## Determinism, precisely scoped

The promise is layered, and being explicit about the layers is what keeps it honest.

- **The note stream is bit-exact, always.** For a given score, seed, tempo map and tick stream, the sequence of `NoteEvent` values is identical across compilers, platforms, backends and runs.
  It is computed entirely in integers and fixed point by pure functions, and it is what a replay reproduces.
- **The command stream is bit-exact, always.** Sample timestamps come from exact integer maps, so which frame a note lands on is reproducible even though the audio thread is not.
- **Antwika's own rendered samples are bit-exact for a fixed build.** Same binary, same score, same buffer — same bytes.
  Across compilers it is *practically* identical but not promised, because IEEE-754 leaves an implementation latitude in transcendental functions and in expression contraction; a build compiled without fast-math and without FMA contraction closes most of that gap, and pinning it in the offline test build closes the rest.
- **Rendered samples with third-party plugins are not promised at all**, for the reasons above.

Where a test needs a number rather than a promise, it asserts on the note stream or on a hash of it, and reserves exact-buffer comparison for the built-in voices in the pinned offline build.

## Testability

Every layer is testable without a sound card, and the ordering of the phases is chosen so that each new layer arrives with its test harness already present.

- **A pattern is a pure function.** `EXPECT_EQ(collect(pattern, bars(4, 8)), expected)` — no device, no thread, no clock, no mock.
  Euclidean rhythms have published outputs to check against, and every transformation has an algebraic law worth asserting: `retrograde(retrograde(p)) == p`, `transpose(a, transpose(b, p)) == transpose(a + b, p)`, `stretch(1/1, p) == p`, and `stretch(r, stretch(s, p)) == stretch(r * s, p)`.
- **Positional randomness is testable out of order**, which is the point: asserting that querying bars 400 to 404 directly gives the same events as querying bars 0 to 404 and filtering is a one-line test that would fail instantly against a stateful RNG.
- **A `TempoMap` is checked for exactness**, not for closeness: `pulseAt(samplesAt(p)) == p` at every segment boundary, monotonicity across a fuzzed sequence of tempo changes, and no drift over an hour of pulses.
- **Voices and the mixer render offline.** `OfflineDevice` fills a caller-owned buffer synchronously on the test thread, so a synth test is "render four bars and assert".
  Assert on structure first — where the zero crossings are, where the envelope's knees fall, that silence is exactly silent, that a note-off at frame N leaves nothing after N plus the release — and on exact bytes only where the build is pinned.
- **The ring buffer gets its own tests plus a ThreadSanitizer run**, since it is the one genuinely concurrent thing in the tree and the only place where a data race is possible at all.
- **The audio-thread rules get a checker script**, so "no allocation on the audio thread" is enforced rather than remembered.
- **The device seam gets a conformance suite**, exactly as the graphics and input seams have: open, start, render a known number of frames, stop, and assert the frame counter is monotone and the callback was never re-entered.
  Every backend runs the same suite, and `null` runs it fastest.
- **The whole thing runs headless in CI** with `ANTWIKA_AUDIO_BACKEND=null`, and under SDL with the dummy audio driver, which is the audio analogue of `SDL_VIDEODRIVER=dummy`.

## Roadmap

Each phase is independently shippable and independently testable, and each one ends with something a test can assert about.
No phase depends on the phase after it.

**Phase 1 — musical time and the pattern algebra.**
`Pulse`, `PulseSpan`, `Ratio`, `TempoMap`, `Pitch`, `Velocity`, `NoteEvent`, `IPattern`, `IPatternSink`, the sixteen Phase 1 combinators, and `ScoreError`.
No sound, no thread, no device, no backend, and no dependency beyond `antwika::time`.
Needs `antwika::rng` lifted out of `holdem` for the positional hash, which is a small job worth doing anyway.
Ships as a library with a complete unit test suite and the algebraic laws asserted.

**Phase 2 — envelopes, parameters and modulation.**
`ParamId`, `ParamValue`, `IEnvelope`, `Ramp`, `Curve`, `AdsrEnvelope`, `LoopedEnvelope`, `Modulation`, `ModulationMatrix`, and the density-gated pattern that proves a pattern parameter and an instrument parameter are the same thing.
Still no sound.
Ships as pure values.

**Phase 3 — the device seam and the null backend — shipped, as `antwika::sound`.**
`SampleBuffer`, `IRenderCallback`, `IDevice`, `ISoundBackend`, `SoundCapabilities`, `SoundError`, `makeSelectedSoundBackend()`, `NullSoundBackend`, the `ANTWIKA_SOUND_BACKEND` CMake variable, the `sound_backend` Conan option and the backend conformance suite all exist.
`sound_backend` defaults to `null` rather than following `gfx_backend`, which is where it parts company with `input_backend`; [`wiki/libraries/sound.md`](../wiki/libraries/sound.md) says why.

**Phase 4 — voices, the mixer, and offline rendering — half shipped.**
`Waveform`, `WaveformLibrary`, `PlayRequest`, the fixed voice pool of `Mixer`, `WavReader` and `OfflineDevice` are built: a session can be rendered to a waveform and asserted sample by sample with no hardware at all.
Outstanding: every piece of DSP — a sample-time `Adsr`, a band-limited oscillator, a one-pole filter — plus a WAV *writer*, which belongs under `antwika::app` rather than in the library since the library opens no files.
So a sampler you can hear exists; a synthesiser does not.

**Phase 5 — the real-time seam — the part that survives has shipped.**
The pumped device model removed the reason for most of this phase, as the [device seam](#the-device-seam) section explains: with no thread of ours there is nothing for an SPSC ring to cross.
What survived was the `sdl3` target beside the existing graphics and input ones, and the pacing rule, and both are built: `backends/sdl3` implements the seam, and `apps/sound_demo` paces itself against `framesPlayed()` so a track takes as long to run as it takes to hear.
What is deferred until a backend genuinely needs a callback thread is the ring itself, the lookahead sequencer, the overflow counter, the denormal guard and `scripts/check_audio_thread_purity.py`.
What is still missing before this plays *from a tick loop* is the sequencer, which needs Phase 1's musical time -- so the remaining work here is upstream of this phase rather than in it.

**Phase 6 — the script front-end and a showcase app.**
A parser for the syntax above, `ScriptParseError`, and `apps/sequencer`: a window drawing the arrangement and the active voices through `antwika::gfx`, transport controls through `antwika::ui`, key bindings through `antwika::input::ActionMap`, and the whole session recorded and replayed through `--record`/`--replay` like every other app in the tree.
Only externally-supplied events are persisted — a key press, a tempo change, a script reload — and every note is regenerated, which is the same rule `apps/poker` follows for its cards.

**Phase 7 — CLAP hosting.**
`IPluginHost`, `IPluginInstance`, `PluginError`, `backends/clap/`, the `audio_plugins` Conan option defaulting to `none`, and the projection-side-only rule enforced by there being no API through which a plugin could return anything to the score.

**Phase 8 — Antwika as a CLAP plugin.**
The inverted loop, transport-driven ticks, state serialisation via the Phase 6 script format, and `clap-wrapper` for a VST3 and AU build.
This is a product rather than a demo, and it should not be started until Phases 1 through 6 have been used in anger.

## Open questions

- **Is `Q16.16` enough for parameter values, or is a wider fixed-point type needed for a cutoff in hertz?** The alternative is a per-parameter unit and range, which is more honest and more code.
- **Should `antwika::audio` be allowed to depend on `antwika::ecs`?** The recommendation is no, mirroring `antwika::ui`, with the sequencer sink living in the application; but a voice pool is a very natural ECS `World`, and that argument deserves a hearing.
- **How is a live MIDI keyboard recorded?** The position taken here is that MIDI input is *input* and belongs behind an `antwika::input`-shaped seam so that it arrives as edges in the tick stream, but that means a MIDI note is quantised to a tick, which a performer will notice at 60 Hz.
- **What is the right tick rate for music?** 60 Hz gives about 17 ms of scheduling granularity, which is fine for scheduling ahead and coarse for live input; running the audio app at a higher tick rate is legal but makes its replays incomparable with other apps'.
- **Does a ring overflow drop the command or stall the sequencer?** Dropping is the only audio-safe answer and it means a lost note, so the ring must simply be sized so that it cannot happen, and the counter exists to prove it did not.
  Not live while devices are pumped, since there is no ring.
- ~~**Where do sample assets live, and who decodes them?**~~ **Answered.**
  `sound::WavReader` decodes, from a `std::istream` rather than a path, exactly as `gfx::PngReader` does, so the library opens no files and an application supplies the bytes.
  Streaming a long file from disk still does not fit that shape and is still unanswered, but nothing yet needs it: a `Waveform` is decoded whole.
- **Should the score be serialisable independently of the script?** Phase 8 needs it for plugin state, and a JSON score would also let the replay format carry a score inline rather than by path.
- ~~**Is a second thread acceptable at all in a repository built on single-threaded determinism?**~~ **Not yet asked.**
  The pumped device model means the question has not had to be answered: `antwika::sound` renders on the thread that pumps it, and the whole test suite runs with no thread, no device and no wall-clock time.
  It becomes live again only if a backend arrives that cannot be pumped, which is what `SoundCapabilities::selfDriven` exists to let one say.
