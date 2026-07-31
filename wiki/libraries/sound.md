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

## Backend selection

`sound_backend` (Conan) and `ANTWIKA_SOUND_BACKEND` (CMake), with values `null` and `sdl3`.
It defaults to `null` and deliberately offers no `auto`: [`input`](input.md) follows graphics because a window nobody can click is useless, while sound is orthogonal, and following would mean every existing `sdl3` build silently began opening an audio device.

`raylib` is absent from the values because it does not implement this seam, so Conan refuses it before anything is downloaded.

The SDL3 backend claims SDL's audio subsystem and nothing else, so a build selecting it for sound alone never asks for a display — which is why CI runs the sound conformance suite **without** `xvfb-run` while running the graphics and input ones under it.

## See also

- [`docs/sound.md`](../../docs/sound.md) — the long-form argument.
- [`docs/audio-player-plan.md`](../../docs/audio-player-plan.md) — the musical layer this leaves room for.
- [sound_demo](../apps/sound_demo.md) — the showcase.
