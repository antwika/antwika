# antwika::synth

`src/libs/synth/` — making sound up rather than reading it back.

## What it is for

**Replacing a `.wav` in the repository with a value in a header.**

A sound here is a `VoiceDesc`: a shape, a frequency, a slide, an envelope, a filter, a gain and a pan.
It is a plain value somebody writes in source, reviews in a diff and merges in a branch, rather than bytes exported from a tool nobody else has installed.

[`sound`](sound.md) owns the device, decodes a file and mixes waveforms it was handed.
This library sits above it and hands it none: it generates.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Waveshape.hpp` | `Waveshape`, `isPeriodic()`, `waveshapeName()` | Which shape, and whether a frequency means anything for it. |
| `Oscillate.hpp` | `oscillate()` | One shape sampled at one instant, purely. |
| `Adsr.hpp` | `Adsr`, `envelopeAt()` | The amplitude shape, in frames, as a pure function. |
| `Filter.hpp` | `FilterMode`, `FilterDesc`, `FilterCoefficients`, `FilterState` | A state-variable filter, split into what a person wrote and what the arithmetic needs. |
| `VoiceDesc.hpp` | `VoiceDesc` | **One sound, written down.** |
| `TriggerRequest.hpp` | `TriggerRequest` | One voice, at an **absolute** start frame. |
| `SynthMixer.hpp` | `SynthMixer`, `SynthMixerDesc` | A fixed voice pool that generates into a buffer. |
| `SynthError.hpp` | `SynthError` | This library's one failure type. |

## Depends on

[`sound`](sound.md) and [`rng`](rng.md), and nothing else.

It does not depend on [`time`](time.md), and deliberately: nothing here knows what a tick is.
A voice is placed at a frame, and working out which frame a musical decision belongs on is a sequencer's job rather than this library's.

## Non-obvious decisions

**There is no separate sound-effect subsystem, and that is the central claim.**
The parameter set an sfxr-style effect needs -- a shape, an envelope, a frequency and a slide -- **is** one synthesised voice, and a note of music is one too.
So an explosion and a bassline are two `VoiceDesc` values differing in their numbers and in what decided to fire them, and they meet the same pool through the same `trigger()`.
That is what makes generating voices the right call rather than baking each effect into a `Waveform` once at startup: baking works for effects and runs out of memory for a soundtrack, and keeping both would be two code paths that drift.

**`SynthMixer` mirrors `sound::Mixer` method for method.**
Same fixed pool sized in the constructor, same round-robin stealing, same absolute start frame, same `noexcept` render that allocates nothing.
Because it is an `IRenderCallback`, every device, backend and conformance test in [`sound`](sound.md) takes it unchanged, and that library gained nothing at all to make this one possible.

**Every refusal is in `trigger()` and none is on the render path.**
By construction rather than by care: a voice in the pool was checked when it was triggered, and its filter coefficients were worked out there too.
So `render()` has nothing to validate, nothing to look up, nothing to allocate and no transcendental to call -- one `std::sin` per note instead of one per sample is exactly why `FilterCoefficients` is a separate type from `FilterDesc`.

**Noise is positional, not generated.**
`oscillate()` hashes the seed and the voice's own elapsed frame rather than advancing a shared generator.
A shared one would make each voice's output depend on how many others were sounding and on the order they were stolen in -- a divergence that shows up only under load and never in a test with one voice in it.
Hashing the position means a voice's noise is the same whichever others exist, and the same on every run.
[`rng::SplitMix64Rng`](rng.md) does the mixing, constructed per sample and discarded, so nothing is carried between calls.

**The envelope is a pure function of how far a voice has got.**
Not a stage machine something advances, for exactly the reason [`animation`](animation.md) has no `Animator` and [`tween`](tween.md)'s `ease()` is pure: a stage held between calls cannot be asked about out of order and cannot be asserted on without being driven there first.

The release always begins at `hold`, whatever the attack and decay were doing when it arrived, and falls from whatever level had been reached.
So an envelope whose attack outlasts its hold is not an error -- it is a voice cut off while still rising, which is what a short percussive effect actually is.
That is also why `VoiceDesc::totalFrames()` is `hold + release` and is not lengthened by a long attack.

**A cutoff is refused against the Nyquist frequency and clamped against the arithmetic, and those are two different checks.**
`trigger()` refuses a cutoff at or above half the rate, which is a statement about what a caller meant.
`filterCoefficientsFor()` clamps the ratio at a sixth, which is a statement about what a Chamberlin filter can survive: its coefficient is `2 sin(pi r)` and it runs away past one.

**A slide steep enough to take the pitch below zero holds at zero.**
Running the phase backwards would sound like the effect reversing part way through, which is never what a falling sweep meant.

## What this does and does not promise

The samples are bit-exact for a fixed build and only practically identical across compilers, since IEEE-754 leaves latitude in transcendentals and in expression contraction.
A test needing a number therefore asserts on placement and on shape -- which frame a voice starts on, that a tail reaches exact silence, that a filter reduces what it should -- and reserves exact-buffer comparison for a pinned build.

Audio remains a write-only projection in exactly rendering's sense: no sample, phase, envelope stage or filter memory is ever read back into a tick.

## Testing

`sound::OfflineDevice` renders a mixer into a `Waveform`, so the whole library is exercised with no device open and no wall-clock time spent.
`SynthMixerTest` leans on a square wave at one hertz, which stays in its first half-cycle for longer than any test renders and therefore produces exactly plus one every sample -- which is what lets placement be asserted exactly rather than within a tolerance.

## See also

- [`sound`](sound.md) — the device, the mixer this one mirrors, and the musical layer this is the first half of.
- [`docs/codified-music-plan.md`](../../docs/codified-music-plan.md) — the plan this library is stage one of, while it is still unbuilt.
