# music_editor

`src/apps/music_editor/` — a page of code you type into while it plays.

## What it is

A live-coding editor: one pane of mini-notation, always sounding, and every keystroke lands in the music without anything being reloaded, stopped or told to reload.

```
// type at me: every keystroke lands in the music
$: bass 0 ~ 0 [~ 3]
$: lead <12 7> ~ 10 ~
$: bell ~ 19 ~ [24 19]
$: drum 0(3,8)
```

The document is code rather than a box per voice.
A blank line, and a line opening with `//`, is passed over.
A voice line opens with `$:`, then names one of the four voices -- `bass`, `lead`, `bell` or `drum` -- and what follows that is the mini-notation the voice plays.
Each voice has a preset of its own -- a filtered saw, a square, a sine bell and a noise hit -- and a number in a line is a semitone above that preset's base.

Escape pauses and resumes, Enter is a new line, Tab indents by two, and the two buttons pause and silence everything.
A line is refused when it does not open with `$:`, when it names no voice, when it claims a voice a line above it already claimed, or when what it plays is something [`notation`](../libraries/notation.md) or [`pattern`](../libraries/pattern.md) will not have.
Refusals are listed under the pane by line number, at most three at a time and then a count of the rest.
A voice no line names falls silent, since deleting a line is how an instrument is taken out.

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

[`notation`](../libraries/notation.md) reads a voice line, [`pattern`](../libraries/pattern.md) is what it reads into, [`sequencer`](../libraries/sequencer.md) turns that into things beginning at exact frames, [`synth`](../libraries/synth.md) makes the sound and [`sound`](../libraries/sound.md) plays it.
[`ui`](../libraries/ui.md) draws it -- the pane is one `ui.textArea()`, the multi-line half of its text field -- and [`input`](../libraries/input.md) is where the typing comes from.

## Non-obvious decisions

**This app defines no event of its own.**
Every bit of its state -- the document, the caret, whether it is paused -- is derived from key and pointer edges the recording already carries, so a replay retypes the session rather than replaying its text.
That is the "only externally-supplied input is persisted" rule taken to its conclusion: there is nothing here that is not worked out again.

**A voice is named rather than counted.**
The syntax could have been positional -- the first line is the bass, the second the lead -- and that is exactly what breaks while somebody is typing into the middle of a score: writing one line above another would move that other one to a different instrument, and the whole document would change sound at the moment a newline was pressed.
Naming the voice makes a line's instrument a property of the line rather than of where it sits, so a document can be reordered, commented out and written into freely.
It also gives deleting a line a meaning -- the voice it named falls silent -- and gives two lines claiming one voice something to be refused for.

**A line that will not parse keeps playing whatever it last did.**
Half a bracket is typed on the way to a whole one, and an editor that fell silent at every intermediate keystroke would be unusable.
The refusal is reported through `Score::problems()` instead, named by the line it belongs to, and the new pattern takes over the moment the line reads again.
`Score` is where that lives, and it is the whole of why live editing feels live.

**Escape pauses, and Enter does not.**
Enter is what a line break is written with, which a document of many lines cannot do without -- so the pause had to move to a key the writing does not need.
Escape is that key, and it is deliberately *not* handed to [`ui`](../libraries/ui.md) as `Key::Cancel`: a field that gave up on what was typed would throw away the score being written.
`EditorSink` reads `input::Key::Escape` itself, before the UI is described, which is also why no `ui::Key` for it has to exist.
Tab moved for the same reason: there is one thing to type into, so it has no focus to walk, and it indents by two spaces instead.

**The text is drawn at twice the glyph scale.**
What is being read here is code, and a mis-read bracket is a line that will not play -- this is also the window in the tree that is looked at for the longest at a stretch.
`editorTheme()` doubles `Theme::textScale` and nothing else, rather than reaching for a scaled theme: doubling every inset and every padding as well would spend the window on its own margins.
The window is 1120x640 because the doubled glyph and a document rather than a row of fields both want more room than four one-line boxes did.

**Pausing stops the musical clock, not the device.**
A held note rings out, the device never starves, and the frames that went by while paused are counted -- so resuming does not decide notes for a moment already rendered.
That counter is the one piece of arithmetic in the app that is easy to get wrong and impossible to hear until it is.

**The run is paced by how much audio the device has taken.**
That is the one thing `IDevice::framesPlayed()` is allowed to decide, and it gives the app a property worth having: a device that consumes the moment it is pumped is never ahead, so a `null` or offline run costs no wall-clock time at all, while a real one is paced by the hardware rather than by a second clock with an opinion of its own.

**One sequencer per track**, because a track's events have to become a voice through *its* preset, and [`sequencer`](../libraries/sequencer.md)'s seam deliberately hands on controls rather than sounds.

**Its words are fixed English literals rather than an `i18n::Translator`.**
The layout is a function of the words, so a session recorded in one language and replayed in another would resolve a click to a different widget.
[ui_demo](ui_demo.md) answers that by fixing the locale in `main()`; this app answers it by having one language, which is the same guarantee with nothing to configure -- and what is actually being written here is mini-notation, which is not English to begin with.
The voice names are the same literals seen from the other side: `bass`, `lead`, `bell` and `drum` are what a document is written with, so translating them would change what the document means.

## See also

- [`notation`](../libraries/notation.md) — the grammar a voice line is written in.
- [`ui`](../libraries/ui.md) — the text area the document is typed into.
- [sound_demo](sound_demo.md) — the same device seam, with no editing over it.
