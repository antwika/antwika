# music_editor

`src/apps/music_editor/` — a page of code you type into while it plays.

## What it is

A live-coding editor: one pane of code, always sounding, and every keystroke lands in the music without anything being reloaded, stopped or told to reload.

```
// type at me: every keystroke lands in the music
$: bass.n("0 ~ 0 [~ 3]").o(-1)
$: lead.n("<12 7> ~ 10 ~").gain(.18)
$: drum.n("0(3,8)")
$: drum.n("~ [0 0] ~ 0")
    .gain(.12).pan(.5)
```

A blank line is passed over, and so is everything after a `//` -- on a line of its own or after a voice, since a comment is cut off wherever it starts.
The one place `//` is not a comment is inside an `n("...")`, where it is the notation's to refuse.
A voice line opens with `$:` and carries a chain of calls joined by dots, optionally opening with the name of a preset to start from.
`n("...")` is the one call every voice needs: what is inside it is the [mini-notation](../libraries/notation.md) that voice plays, and a number in it is a semitone above the voice's base pitch.

**A line is a voice, and nothing is limited to one of a kind.**
The two `drum.` lines above are two voices sounding together, because a preset is a *starting point* that the calls after it change a copy of.
Nothing is stopping four bass lines, or a document with no preset named anywhere in it.

Escape pauses and resumes, Enter is a new line, Tab indents by two, F10 fills the screen, and the two buttons pause and silence everything.
Refusals are listed under the pane by line number, at most three at a time and then a count of the rest.

**It writes like a text editor**, which is to say the parts of one a score needs and no more:

| | |
| --- | --- |
| Left click | Put the caret where you clicked; below the last line is the end of the document. |
| Drag, or shift and click | Select from wherever the caret was to where the pointer is. |
| Shift and an arrow | Select a character or a line at a time. |
| Home, End | Put the caret at the line's start or end; with shift, select that far. |
| Ctrl+C, Ctrl+X, Ctrl+V | Copy and cut to the system clipboard, and paste whatever it holds -- from this editor or any other program. |
| Backspace, Delete | Take one character, or the whole selection when there is one. |
| The wheel, or the bar on the right | Move the pane three lines a notch, or as far as you drag it. |

The pane keeps the caret in view while you type and stays where you put it while you scroll.
It holds as many lines as it has room for and no more: the score can be longer than the window.

## The calls

Every call takes exactly one argument.

| Call | What it does |
| --- | --- |
| `n("0 ~ [3 5]")` | What the voice plays, in mini-notation. A voice with no `n(...)` is refused. |
| `s(saw)` | The oscillator: `sine`, `saw`, `square`, `triangle` or `noise`. |
| `base(440)` | What a note of zero sounds at, in hertz. Never read for `noise`. |
| `o(-1)` | Octaves up or down, which is twelve semitones a time. |
| `trans(7)` | Semitones up or down. Adds to whatever `o()` asked for. |
| `gain(.25)` | How loud, between -1 and 1. |
| `pan(-.4)` | Where it sits, -1 hard left to 1 hard right. |
| `att(4)`, `dec(120)`, `rel(90)` | The envelope's attack, decay and release, in milliseconds. |
| `sus(.6)` | What it holds at after the decay, between -1 and 1. |
| `hold(400)` | The longest this voice rings, however long its note is. A drum is a hit whatever slot it lands in; a bass note fills its slot. |
| `lpf(900)`, `hpf(4000)`, `bpf(1200)` | A low, high or band pass filter at that many hertz. |
| `res(.8)` | How much the filter emphasises its cutoff. One is flat and smaller is sharper. |
| `slide(-40)` | How fast the pitch moves, in hertz per second. |

The presets a line may open with are `bass` (a filtered saw), `lead` (a square), `bell` (a sine) and `drum` (a filtered noise hit).
They are four points to start from rather than four instruments, and a document naming none of them is an ordinary document.

## Some scores

Each of these is a whole document.

**A first beat.**

```
$: drum.n("0 ~ 0 ~")
$: bass.n("0 ~ ~ 0")
```

**Three drums, out of one preset.**
The point of the language: `drum` is where each of them starts, and the calls after it are what make them different.

```
$: drum.n("0(3,8)")
$: drum.n("~ ~ [0 0] ~").gain(.15).pan(.6).hpf(5000).dec(20)
$: drum.n("~ 0").gain(.1).pan(-.6).lpf(600).hold(120)
```

**A voice built from nothing at all**, naming no preset.

```
$: n("0 [3 7] 12 <10 5>").s(triangle).base(220).att(2).dec(90).sus(.2).rel(120).lpf(1800).res(.5).gain(.25)
```

**The same voice, down as many lines as it likes.**
A line opening with a dot is the one above it carrying on, so this is the line above written out and sounds identically:

```
$: n("0 [3 7] 12 <10 5>")
    .s(triangle).base(220)
    .att(2).dec(90).sus(.2).rel(120)
    .lpf(1800).res(.5)
    .gain(.25)
```

The dot opens the continuation rather than ending the line above it, so a chain being written stays legible while it is half typed -- every line of it reads as a call -- and adding one never means editing the line before it.
A voice spread this way is refused or sounded as one, and named by the line its `$:` is on.

**One pattern, two sounds.**
The same notation twice, a fifth apart and panned either side, is a chord that neither line contains.

```
$: lead.n("<0 3 7>*2").gain(.16).pan(-.4)
$: lead.n("<0 3 7>*2").trans(7).gain(.12).pan(.4).s(saw)
```

**A pad under a bass.**
Long envelopes are what make a voice a pad; nothing else about it differs.

```
$: n("<0 5 7 3>").s(saw).base(110).o(1).att(400).dec(600).sus(.6).rel(900).hold(2000).lpf(1200).res(.4).gain(.12)
$: bass.n("0 ~ <0 -5> ~").o(-1).gain(.3)
```

**Cross-rhythm**, which is [`notation`](../libraries/notation.md) doing the work rather than this app.

```
$: drum.n("0*4").gain(.2)
$: drum.n("0(5,8)").gain(.14).pan(.4).hpf(6000).dec(15)
$: bell.n("[0 7 12]/2").gain(.16)
```

**What a refusal reads like.**
Every one of these lines is refused, and each names what it wanted.
The comments are the messages the editor actually prints under the pane:

```
$: bass.n("0").wobble(3)     // wobble() is not a control: n s base o ...
$: piano.n("0")              // piano is no preset: bass lead bell drum
$: n("0").s(kazoo)           // s(kazoo) names no shape: sine saw square ...
$: n("0").gain(x)            // gain(x) wants a number
$: bass.o(1)                 // a voice needs an n("...") to play
$: bass.n(0 3)               // n(...) wants its notation in quotes
  .gain(.2)                  // a call above no voice line
```

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

It runs until the window is closed or a replay dispatches `engine.stop`, exactly as [game](game.md) does -- an editor is sat in for as long as the writing takes, and one that walked out after two minutes would be a demo rather than a tool.
The default `null` backend reports no close, so `Ctrl+C` is what ends one there, and a `--record` run killed that way keeps everything up to the kill.
`--record` and `--replay` work as they do everywhere else.

## What it is built from

[`notation`](../libraries/notation.md) reads what is inside an `n(...)`, [`pattern`](../libraries/pattern.md) is what it reads into, [`sequencer`](../libraries/sequencer.md) turns that into things beginning at exact frames, [`synth`](../libraries/synth.md) makes the sound and [`sound`](../libraries/sound.md) plays it.
[`ui`](../libraries/ui.md) draws it -- the pane is one `ui.textArea()`, the multi-line half of its text field -- and [`input`](../libraries/input.md) is where the typing comes from.

## Non-obvious decisions

**This app defines one event of its own, and its name says why.**
Every bit of its state -- the document, the caret, whether it is paused -- is derived from key and pointer edges the recording already carries, so a replay retypes the session rather than replaying its text.
The exception is `music.paste`: what the system clipboard held is externally supplied and cannot be worked out again, so it is persisted, which is the "only externally-supplied input is persisted" rule read in both directions.

**A voice is a line, and a preset is only where it starts.**
The obvious design is four instruments a line asks for by name, and it is wrong in one specific way: it makes "two drums at once" inexpressible, and two drums at once is most of what a drum part *is*.
So a preset is a value a line takes a copy of, every call after it changes that copy alone, and how many voices there are is how many lines there are.
It also settles what a voice is identified by, which matters while somebody types: nothing.
A line is its own voice, deleting it takes that voice out, and writing one above another moves neither of them to a different instrument.

**A call changes a copy, and the chain is read left to right.**
`bass.o(-1).o(-1)` is two octaves down, because `o()` and `trans()` accumulate into one semitone offset rather than setting it -- that is the one call where "left to right" is visible, and it is what lets `.o(1).trans(-2)` mean what it looks like.
Every other call sets what it names, so a chain naming `gain` twice ends on the second.

**A line that will not parse keeps playing whatever it last did.**
Half a bracket is typed on the way to a whole one, and an editor that fell silent at every intermediate keystroke would be unusable.
The refusal is reported through `Score::problems()` instead, named by the line it belongs to, and the new pattern takes over the moment the line reads again.
`Score` is where that lives, and it is the whole of why live editing feels live.

**The keyboard is a table this app keeps, and it is Swedish by default.**
[`input`](../libraries/input.md) reports *where* a key is rather than what it types, so what a key types has to be written down somewhere, and `EditorKeys` is where.
That is not a detail for a score: `$:` opens every voice line, and shift and the full stop is a colon on a Swedish board and a greater-than on an American one, so reading the wrong table makes the language untypeable rather than merely odd.
Both tables are here, the box above the pane switches between them, and the Swedish one is the default because that is the board this is written on.
Every character the notation needs is reachable on both -- on the Swedish board the dollar and the brackets are on the right-hand alt key, which is where that board really keeps them.
A key whose character [`gfx`](../libraries/gfx.md) cannot draw types nothing at all: the coverage is printable ASCII, so å, ä, ö and the dead accents are absent, and no score is written in them in any case.

**The clipboard is the system's, read and written where each direction is lawful.**
What a desktop clipboard holds is not in any recording, so a sink reading one would paste whatever the replaying machine happened to have and diverge from the run it exists to reproduce.
So the read happens where a key press is read: `PasteSource`, above the loop and upstream of the recorder, sees `Ctrl+V` and says what `input::IClipboard` held as a `music.paste` event -- the one event this application defines, because pasted text is the one thing here that cannot be worked out again.
The recording carries the characters, a replay reads no clipboard at all, and the sink types the payload into `ui::Keyboard::typed` with a `Key::Character` edge each, so [`ui`](../libraries/ui.md) never learns that a clipboard exists.
The write goes the other way on the render side's terms: a copy lands in `EditorState` as simulation state and a live run's sink mirrors it outward, an outward write no tick reads back -- and a replay is handed no clipboard to write, so replaying somebody's session leaves this machine's alone.
`input::makeSelectedClipboard()` is the seam: SDL3's clipboard on that backend, raylib's on that one, and a string in the process under `null`, so a headless run still pastes what it copied.

**Every event describes the editor, acts, and describes it again.**
That is the remedy `ui::Context::finish()` gives for its own ordering: a press is resolved while the frame is being laid out, so the picture beside it predates whatever the press changed.
The second description is given no keys and no press, or a keystroke would be typed twice and a button activated twice, and only the first frame's answers are read.
It costs one more layout and no retained state, and it is what makes a click's caret, a scroll and a typed character all show up in the frame they happened in.

**How far a pane can scroll is `ui`'s answer rather than this app's.**
`EditorState::scroll` is the line at the top of the pane -- simulation state, since it decides which line a click lands on -- but how many lines fit is a function of the arranged layout, which the tick path deliberately cannot see.
So the wheel adds notches to it without bounds, `ui` clamps and reports back through `Interactions::scrolled`, and the app stores that.
A wheel spun a hundred notches past the end settles on the last page after exactly one frame.

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

**One sequencer per voice line**, because a line's events have to become a sound through *its* preset, and [`sequencer`](../libraries/sequencer.md)'s seam deliberately hands on controls rather than sounds.
The pool grows as lines are written and is kept when they are deleted, since a few hundred bytes is cheaper than an allocation in the middle of a bar.
A sequencer taken up again -- because a line was written where a deleted one used to be -- calls `Sequencer::joinAt()`, which is the one thing this app needed the library to grow: a sequencer built fresh has been asked nothing, so its first `advance()` would query every cycle since the run began and sound the lot at once.
Joining says the past is not this voice's to play.

**Its words are fixed English literals rather than an `i18n::Translator`.**
The layout is a function of the words, so a session recorded in one language and replayed in another would resolve a click to a different widget.
[ui_demo](ui_demo.md) answers that by fixing the locale in `main()`; this app answers it by having one language, which is the same guarantee with nothing to configure -- and what is actually being written here is mini-notation, which is not English to begin with.
The preset and control names are the same literals seen from the other side: `bass`, `gain`, `lpf` and the rest are what a document is written with, so translating them would change what a document means.
The keyboard box is the one place a language appears at all, and it names a *board* rather than a tongue: `swedish` there is which key types a colon, not what the window is written in.

**Filling the screen is not simulation state, so F10 is above the loop.**
`app::FullscreenToggleSource` wraps the event source and toggles the window on the key, and it changes exactly one thing -- what `IWindow::size()` reports -- which no layout, no hit test and nothing this app plays may read.
What *does* read it is the render sink alone, to place the picture and nothing else: a `gfx::ViewportRenderer` built fresh each frame scales the canvas to the window's full height at the same aspect and pillarboxes the remainder, exactly as [game](game.md) draws, and `app::WindowPointerMapping` runs the same transform backwards upstream of the recorder so a click lands on what it is over at any size.
Every layout here is against `configuredSize()`, so a recorded session reaches the same state whether or not anybody pressed it, and a replay of one where somebody did fills the screen at the same tick and still reaches that state.
It is the same key [game](game.md) uses, since an editor with a different one would be one to remember.

## See also

- [`notation`](../libraries/notation.md) — the grammar a voice line is written in.
- [`ui`](../libraries/ui.md) — the text area the document is typed into.
- [sound_demo](sound_demo.md) — the same device seam, with no editing over it.
