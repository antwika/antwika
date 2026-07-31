# The sound library owns no thread and no clock

`antwika::sound` decodes PCM audio, mixes it, and hands it to a device, and it does all three on whatever thread called it.
There is no audio thread, no lock, no ring buffer and no atomic anywhere in the library.
A device renders when a caller asks it to, through `IDevice::pump()`, and does nothing at all in between.

This document is about why that is the design rather than a stage it has not reached yet, because a callback on a framework's own high-priority thread is what almost every audio library ships, and it is the shape that would cost this project the most.

## Two rules it is serving

The first is the one every library here serves.
`antwika::simulation::EngineLoop` is the one code path shared by a live run and a replay, and what makes a replay reproduce a session is that only externally-supplied input is persisted.
A sound is not input: which sound plays at tick 4,096 is a consequence of the tick and the state, so it has no business being in a recording, and the way to keep it out of one is to leave the library nothing that could put it there.

The second is narrower and is the reason for the threading answer.
A real audio callback runs on a thread the framework owns, at a priority the framework chose, and blocking it is what a listener hears as a click.
So the usual arrangement is a lock-free queue between the simulation and the callback, which is a second concurrency model in a codebase that otherwise has none, and a whole class of failure that only shows up under load on somebody else's machine.

Pumping deletes that.
`pump(frames)` renders exactly that many frames and returns, on the calling thread, so the mixer is stepped by the same loop that steps everything else and in the same order every time.
A headless run costs no wall-clock time at all, which is why the whole test suite runs in milliseconds and needs no hardware.

`SoundCapabilities::selfDriven` is what keeps the other door open.
A backend that genuinely cannot be pumped says so, and the conformance suite skips the tests that assume otherwise rather than failing a backend for being honest.
Nothing in the library asserts that a device is pumpable; it asks.

## Why a frame index is absolute

`IRenderCallback::render(SampleBuffer out, FrameIndex firstFrame)` is handed the index of the first frame of the buffer, counted from when the device started, and never a count of frames since the last call.

This is the one interface decision that would be expensive to change later, and it is the thing real devices most often get wrong.
A device that restarts its counter per buffer forces every caller to keep its own running total, and every caller that keeps one has somewhere to lose count -- a dropped buffer, a restart, an underrun -- after which every scheduled sound is placed at the wrong moment.
The bug is inaudible in a short test and obvious after a minute of play.

`SoundBackendConformance`'s `Render_ReceivesAscendingContiguousFrames` is the test that catches it, and it is the test in that suite worth the most.
It asserts that the first call begins at frame zero and that each call begins exactly where the last one ended.

Absolute frames are also what makes `PlayRequest::startFrame` mean something.
A sound placed at frame 48,000 begins at frame 48,000 and not at whichever buffer boundary happens to follow, so placement is sample-accurate without the mixer knowing anything about buffers.
"Play it now" is deliberately not expressible: `now` is a different frame depending on how far ahead the device is rendering, which is exactly the ambiguity the absolute index removes.

## What `framesPlayed()` may be used for

It is monotonic and it is advisory.
It is legal to read to decide how long to sleep, and it is never legal to read to decide what to compute.

The distinction matters because it is the one number in this library that a real device derives from hardware, and hardware does not agree with a tick count.
A mixer that asked "how far has the device got?" before deciding what to play would be a simulation reading a clock, and a replay of that run would compute something else.

## Why a waveform is always float

`Waveform` holds normalised `float` samples whatever the file it came from held, and there is deliberately no sample-format enum anywhere in the library.

An enum would mean a conversion matrix -- every stored width against every device width -- and a set of paths most of which nothing would ever take.
Decoding to one representation at load time deletes the matrix, and it puts the only conversion in the one place a file is read.
`gfx::Bitmap` makes the same trade for the same reason: decode once, to a plain value, and let everything downstream assume one layout.

Float is also the one place in this project where floating point is allowed without argument, because samples never reach simulation state.
Audio is a write-only projection in exactly the sense rendering is: what a run computes decides what is played, and nothing that is played decides what a run computes.

## Planar buffers and interleaved files

`SampleBuffer` is a non-owning span of per-channel spans -- planar -- because that is what a mixer wants: one contiguous run per channel, written with no stride arithmetic and no allocation on the render path.
`Waveform` is interleaved, because that is what a file holds and what a device wants.

`OfflineDevice` is the one place the two layouts meet, and it says so in a comment.
Keeping the crossing to one function is the point: a stride mistake in an audio path is a buzz rather than an error, and there is exactly one function to look at.

## Refusing rather than resampling

A waveform whose sample rate differs from the mixer's is refused, with a message that says the two rates and says that this library does not resample.

Resampling well is a real piece of signal processing and resampling badly is audible, so the honest answer while there is no resampler is to refuse.
A caller that hits this converts the file once, offline, which is where the work belongs anyway.

## What is left to reject

One exception type, `SoundError`, following `gfx::GfxError`, and it is thrown from constructors, from `openDevice()`, from `start()` and from the decoder.

`Mixer::render()` has no error path at all, and that is by construction rather than by care: a mixer that exists was built with a valid format and a voice pool sized once, and every voice in it holds a waveform the library owns, so there is nothing left for the render path to check and nothing for it to allocate.
`WaveformLibrary` owning the waveforms is what makes that true -- a mixer holds pointers into it and never a copy, and there is no way to hand the mixer something that could go away underneath it.

`WavReader` reads from a `std::istream` rather than a path, exactly as `gfx::PngReader` does, and for the same two reasons: the library opens no files, and every refusal it can produce is reachable from bytes in memory.
Every one of them is, which is why the decoder is covered without anything on disk.

## Choosing a backend

`sound_backend` is a Conan option and `ANTWIKA_SOUND_BACKEND` the CMake variable behind it, exactly as graphics and input have.
It defaults to `null` and deliberately does not offer `auto`.

Input follows graphics because a window nobody can click is useless, and one flag driving both is what a caller wants.
Sound is orthogonal to both, so following would mean every existing `-o gfx_backend=sdl3` build silently began opening an audio device -- a device, a subsystem and a dependency nobody asked for.

`raylib` is absent from the option's values because it does not implement this seam.
An unlisted value is the cheapest possible way to say so: Conan refuses it before anything is downloaded, which is a better answer than a link error much later.

The SDL3 backend is a **push** model, and that is what keeps this library single-threaded.
`SDL_OpenAudioDeviceStream` with a null callback starts no thread of ours and calls nothing of ours; the caller renders and hands buffers over with `SDL_PutAudioStreamData`.
So `pump()` on an SDL3 device does what it does on the null one, on the thread that called it.

It claims SDL's audio subsystem and nothing else, so a build selecting `sdl3` for sound alone never asks for a display.
That is worth checking rather than assuming, and it is why the sound conformance suite runs in CI **without** `xvfb-run` while the graphics and input ones run under it: a backend that had quietly taken a dependency on video would pass under Xvfb and fail on a headless machine.

## What it does not depend on

`antwika::log`, and nothing else.
Not `time`, not `ecs`, not `replay`, not `gfx`.

Holding no clock is what leaves room for a musical layer above it, in the way `antwika::animation` holds none: a tempo map, a bar and a beat are all functions of a frame index somebody else is counting, and a library that had started counting would have to be asked to agree.
