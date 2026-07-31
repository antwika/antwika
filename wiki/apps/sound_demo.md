# apps/sound_demo

`src/apps/sound_demo/` — the sound showcase.

## What it demonstrates

[`sound`](../libraries/sound.md) end to end: a waveform, a mixer, a schedule of notes at exact frame positions, and a device pumped from the calling thread.

## Running it

```sh
build/bin/antwika_sound_demo/antwika_sound_demo                  # a generated tone
build/bin/antwika_sound_demo/antwika_sound_demo my-sound.wav     # or play a file instead
```

Under the default `null` backend it renders every frame and plays nothing, which is what makes it safe for a CI leg to run.
Under an `sdl3` build it is audible; use `SDL_AUDIO_DRIVER=dummy` if there is no sound card.
It needs no display under any backend.

## Libraries it composes

[`app`](../libraries/app.md), [`log`](../libraries/log.md), [`sound`](../libraries/sound.md), [`time`](../libraries/time.md), plus the selected sound backend.
Notably not [`engine`](../libraries/engine.md), [`replay`](../libraries/replay.md) or [`gfx`](../libraries/gfx.md): there is no tick loop and no window here.

## How it is put together

`DemoTrack` builds the material — `demoTone()` generates a fading sine, and `demoSchedule()` lays eight notes out at exact multiples of a spacing, panned left to right.
`DemoLoop` opens a device, plays the whole schedule, and pumps until every frame is rendered.

Both are separated from `main.cpp` so that everything worth covering is testable: the whole app is exercised against the null backend with no sound card and no wall-clock time spent.

## Non-obvious decisions

**The schedule is handed over before a single frame is rendered.**
That is what an absolute `startFrame` buys — a note's moment is decided once, and nothing about how the run is pumped can move it.
A "play it now" API would make the answer depend on how far ahead the device happened to be.

**It paces itself against what the device has consumed.**
The loop pumps a chunk, asks `framesPlayed()`, and sleeps if the queue has run more than a few thousand frames ahead — so a track takes as long to run as it takes to hear rather than being dumped into a buffer.
That is the one legal reading of `framesPlayed()`, and nothing the loop computes depends on the answer.

It also waits for what is queued to be heard before closing, or the last second would be discarded on exit.

**That wait is bounded rather than hopeful.**
Its exit condition is a number from the device, so a device that stopped consuming ends the run a little early instead of hanging it.

**The tests supply a device that lags.**
No backend in the tree does: the null one consumes instantly and the offline one writes straight into a waveform, so neither pacing path is reachable through either.
A fake that lags and closes the gap gradually is the shape a real device has, and a second that never closes it is what proves the bound.

**A frame cap rather than running until something ends it.**
The same reason [gfx3d_demo](gfx3d_demo.md) has one: the default `null` build reports no end, and it is the build every CI leg produces.

## See also

- [`docs/sound.md`](../../docs/sound.md) — why the threading and the absolute frame index are the design.
