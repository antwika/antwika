# music_editor

`src/apps/music_editor/` — four lines of text you type into while they play.

## What it is

A live-coding editor: four fields of mini-notation, always sounding, and every keystroke lands in the music without anything being reloaded, stopped or told to reload.

```
bass  0 ~ 0 [~ 3]
lead  <12 7> ~ 10 ~
bell  ~ 19 ~ [24 19]
drum  0(3,8)
```

Tab moves between lines, Enter pauses and resumes, and the two buttons do the same and silence everything.
Each line is one track with a preset of its own -- a filtered saw, a square, a sine bell and a noise hit -- and a number in a line is a semitone above that preset's base.

## Running it

```sh
build/bin/antwika_music_editor/antwika_music_editor
```

The default build selects the `null` backends, which draw nothing, report no input and play nothing.
**For a window and actual sound**, build with the SDL3 backends:

```sh
conan install . -of build-sdl3 -o gfx_backend=sdl3 -o sound_backend=sdl3 \
  -c tools.cmake.cmake_layout:build_folder_vars="['options.gfx_backend']" \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan-sdl3.lock

cmake --preset conan-gfx_backend_sdl3-release
cmake --build build-sdl3 -j24
build-sdl3/bin/antwika_music_editor/antwika_music_editor
```

It runs for a tick budget of 4800 -- about two minutes -- rather than until the window closes, for the reason [ui_demo](ui_demo.md) does: the default backend reports no close, and that is the build every CI leg produces.
`--record` and `--replay` work as they do everywhere else.

## What it is built from

[`notation`](../libraries/notation.md) reads a line, [`pattern`](../libraries/pattern.md) is what it reads into, [`sequencer`](../libraries/sequencer.md) turns that into things beginning at exact frames, [`synth`](../libraries/synth.md) makes the sound and [`sound`](../libraries/sound.md) plays it.
[`ui`](../libraries/ui.md) draws it and [`input`](../libraries/input.md) is where the typing comes from.

## Non-obvious decisions

**This app defines no event of its own.**
Every bit of its state -- the four lines, the caret, the focus, whether it is paused -- is derived from key and pointer edges the recording already carries, so a replay retypes the session rather than replaying its text.
That is the "only externally-supplied input is persisted" rule taken to its conclusion: there is nothing here that is not worked out again.

**A line that will not parse keeps playing whatever it last did.**
Half a bracket is typed on the way to a whole one, and an editor that fell silent at every intermediate keystroke would be unusable.
The refusal is shown beside the line instead, and the new pattern takes over the moment the line reads again.
`Score` is where that lives, and it is the whole of why live editing feels live.

**Pausing stops the musical clock, not the device.**
A held note rings out, the device never starves, and the frames that went by while paused are counted -- so resuming does not decide notes for a moment already rendered.
That counter is the one piece of arithmetic in the app that is easy to get wrong and impossible to hear until it is.

**The run is paced by how much audio the device has taken.**
That is the one thing `IDevice::framesPlayed()` is allowed to decide, and it gives the app a property worth having: a device that consumes the moment it is pumped is never ahead, so a `null` or offline run costs no wall-clock time at all, while a real one is paced by the hardware rather than by a second clock with an opinion of its own.

**One sequencer per track**, because a track's events have to become a voice through *its* preset, and [`sequencer`](../libraries/sequencer.md)'s seam deliberately hands on controls rather than sounds.

**Its words are fixed English literals rather than an `i18n::Translator`.**
A field is as wide as the label beside it, so the layout is a function of the words, and a session recorded in one language and replayed in another would resolve a click to a different widget.
[ui_demo](ui_demo.md) answers that by fixing the locale in `main()`; this app answers it by having one language, which is the same guarantee with nothing to configure -- and what is actually being written here is mini-notation, which is not English to begin with.

## See also

- [`notation`](../libraries/notation.md) — the grammar the lines are written in.
- [sound_demo](sound_demo.md) — the same device seam, with no editing over it.
