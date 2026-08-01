# apps/companion

`src/apps/companion/` — a tamagotchi in a 256-pixel window.

## What it is

`apps/companion` is a tamagotchi in a 256-pixel window: an animal with two needs, one input, and an end it can reach.
What follows is the rules and the numbers, and why they are those numbers.

## The two needs, and the one input

The only input is a left press anywhere in the window, and what it means depends entirely on when it lands.

- **Feed it while it is awake and hungry.** A tap then is a meal: it takes `feedRelief` off the hunger and gives `feedJoy` happiness back.
- **Leave it alone while it is asleep.** A tap then wakes it: it costs `disturbCost` happiness *and* forfeits the rest of that night's recovery.
- **Do not push food at it while it is awake and full.** A tap then is the third violation and the gentlest one: it costs `pesterCost` happiness, leaves the hunger where it was, and is counted as a pestering rather than as a meal.
  Food offered to a companion that does not want any is left uneaten, and offering it anyway is what a companion has to put up with rather than something that never happened -- which is what keeps tapping repeatedly from being a strategy.
- A tap after it has perished is the only one that does nothing, since nothing about a perished companion ever changes again -- unless it lands on the "new pet" button, which is the one press whose meaning depends on *where* it landed.

The three violations all spend the same currency, and what separates them is how much.
Letting it go famished costs one happiness every `starvePeriodTicks`; waking it costs `disturbCost` at once; pestering it costs `pesterCost` at once.
Happiness reaching zero is `PetState::Perished`, and there is no way back from it -- by any of the three.
A perished companion is gone, and the only thing left to do is start a new one.

## The day, the night, and where the clock lives

The day is a function of the tick count and of nothing else:

```
night(t)  =  (t % (dayTicks + nightTicks)) >= dayTicks
```

Falling asleep and waking up are therefore not countdowns that could get out of step with the day, and the sun does not stop for a companion that has perished.
Hunger grows only while it is awake, so a night is a night off from it.
An undisturbed night gives one happiness back every `restPeriodTicks`, and a disturbed one gives none -- the note of that is cleared on the *first tick of each night* rather than at dawn, so a tap in the daylight cannot spoil the night that follows it.

Nothing in `Pet` reads a clock, a locale or a generator.
It is a pure function of how many times `step()` has been called and of when `tap()` was called between them, which is the whole reason a recording of the taps replays to the same animal: a replay is handed no companion to start from, and every bit of the hunger, the sleep and the happiness is regenerated.
A *live* session does carry its companion over from the one before it -- see [The companion between sessions](#the-companion-between-sessions) -- and a replay run neither reads nor writes one, which is what keeps that true.

## The numbers

Every period is written as a number of seconds times `kTicksPerSecond`, rather than as a tick count somebody has to divide in their head, so changing how fast a companion lives is one constant and every need keeps the same relationship to every other one.
`kTickInterval` -- what `PacingSink` waits, and the only place the wall clock appears at all -- is derived from the same constant.

- `kTicksPerSecond` is 20, so a tick is 50 ms -- smooth enough to watch, and coarse enough that every period below is a round number of ticks.
- `kDayTicks` is 20 seconds, which is long enough to get hungry twice in and short enough that a session sees a night without waiting for one.
- `kNightTicks` is 10 seconds, half a day, so the night is a real interval to keep your hands off rather than a moment.
- `kHungerPeriodTicks` is 2 seconds, so `kHungerMax` of 8 steps fills 16 of a day's 20 seconds and a day you ignore ends famished.
- `kHungerThreshold` is 4, half of the maximum, so there is a whole 8 seconds of visible warning before neglect starts costing anything.
- `kFeedRelief` is 4, exactly the threshold, so one well-timed meal buys back the whole warning.
- `kStarvePeriodTicks` is 3 seconds, so a full day spent famished costs about seven happiness -- more than a companion starts with.
- `kRestPeriodTicks` is 4 seconds, so an undisturbed night is worth two or three happiness and attention pays where neglect does not.
- `kDisturbCost` is 2, twice what a meal is worth, because the night is the need with no warning attached to it.
- `kPesterCost` is 1, exactly `kFeedJoy` and half of `kDisturbCost`, so an unwanted meal is the equal and opposite of a wanted one and waking it stays twice the sin.
  From `kHappinessStart` one stray tap costs a sixth of what it has and an undisturbed night gives two or three of it back, so a mistimed tap is recoverable; six in a row are not, which is the point -- tapping at it without pause is now a way to lose rather than a free action.
- `kHappinessMax` is 10 and `kHappinessStart` is 6, above half, so a first mistake is survivable and a second day of them is not.
- `kSayingTicks` is 3 seconds, long enough to read at a glance and short enough that two things said in a row are two bubbles rather than one.
- `kChatterPeriodTicks` is 6 seconds, twice what a line lasts, so the bubble is empty for as long as it is full and the chatter reads as occasional rather than constant.

The balance those numbers add up to is asserted rather than described: `PetTest` runs the shipped configuration for a hundred seconds twice over, once with nobody attending it and once feeding it whenever it asks, and requires the first to perish and the second not to.
The second only ever taps a hungry companion, so it also pins that attentive play never pesters.
A companion left entirely alone dies in the middle of its third day.

## How it is wired

The shape is `apps/tower_defence`'s rather than `apps/life`'s, because a companion is one plain value rather than a grid of entities.

- `TapSink` decodes `input.pointer_*` and calls `Pet::tap()`.
  **The app defines no event for feeding it, for waking it up or for pestering it**, deliberately: a `--record` run persists the press, and whether it landed on a hungry companion, a full one, a sleeping one or a perished one is worked out again on replay from the same press and the same tick count.
  Persisting the meal as well would feed it twice per tap.
- `PetSink` calls `Pet::step()` once per `engine.tick`, registered after `TapSink` so a press is answered by the state the last tick ended with and the step that follows sees the meal.
- `RenderSink` takes a `PetSnapshot` -- an immutable value the scene cannot write -- and hands it to `PetScene`, registered after both so a frame is of the state the tick ended with.
- `PacingSink` is last, which makes the order present-then-wait.

- `ReviveSink` decodes the same presses and starts a new companion when one lands on the button a perished companion is offered, registered *after* `TapSink` so one press can never be a tap and a revival both.
  **It defines no event either**: a `--record` run persists the press, and that it landed on the button -- and that there was a button to land on -- is worked out again on replay.

There is **one layout and one hit-test**, and there is exactly one thing to hit.
A press anywhere in the window is a tap, as it always was; the "new pet" button is the one press whose meaning depends on where it landed, so `companion::PetLayout.hpp` is this app's `life::BoardLayout`.
`reviveButtonRect()` is shared by `PetScene`, which paints that rectangle, and `ReviveSink`, which hit-tests against it, so what somebody sees and what they can press are one rectangle rather than two that agree today.
Everything is laid out and hit-tested against the *configured* window size rather than the size a window reports, for the reason `life::PointerToggleSink` gives about cells: a hit-test is a function of the layout, and a resized window would resolve a recorded press to a different answer.

### Why the animal is rectangles rather than an atlas

The house style is a hand-drawn PNG atlas addressed arithmetically (see [`game-texture-atlas.md`](game-texture-atlas.md)), and it is right when a picture has hundreds of distinct tiles and an artist who is not the programmer.
This window is 256 pixels square and the animal is ten boxes.
An atlas for it would be a checked-in binary, a second contract in `TileAtlas.hpp`'s shape, and a startup check that the file is the size the header says -- all to hold a picture that is shorter written down than described.
Drawing it from `IRenderer`'s rectangles also leaves the whole scene assertable against a mock renderer call by call, which a blit of somebody's art is not, and `PetSceneTest` asserts exactly that.

Everything is laid out on a grid of `companion::kSceneUnits` whole units a side, centred in the canvas, which 256 pixels divides into exactly eight pixels each.
Whole units rather than fractions of the canvas is what keeps every rectangle the same integer on every toolchain.
`main.cpp` derives its window size from that constant times a pixels-per-unit number rather than naming a pixel count that happens to divide by it, so doubling the window is one number and nothing else is left to keep in step.

### The readout

Three lines of text stand on the ground under the animal: `hunger 3/8`, `happy 6/10`, and one line for what the companion is doing -- `awake`, `awake, hungry`, `asleep`, `asleep, woken` or `gone`.
The first two say in words what the two gauges say in bars, and the third says the one thing no bar can, the perished state included.
Every character of it is read off the `PetSnapshot`, so the readout reports the run rather than adding to it: no new state, no event, no clock, and nothing a replay has to reproduce beyond what it already did.

How big the glyphs are is derived rather than chosen: four glyph pixels to a layout unit, so the readout doubles when the window does, and a unit too small for even that still gets the smallest text.
The block is anchored to the *bottom* of the grid with a unit of margin under the last line, rather than to a row of it, so three lines fit whatever a unit turned out to be worth -- which is what keeps a small canvas from printing text off the bottom of itself.
It is drawn last, after the animal or the grave, so anything a later frame puts over the ground stays in front of what is already there.

### The speech bubble

A bubble appears beside the animal from time to time and goes away again after `sayingTicks`.
What it holds is one of a fixed set of lines: four of them are idle chatter (`hello!`, `bored...`, `nice day`, `la la la`) and five say something about the run -- `feed me!` while it is hungry, `yum yum!` for a meal, `im full!` for one it did not want, `shhh!` for being woken, and `zzz...` while it sleeps undisturbed.
A tap is answered on the tick it lands, and otherwise it finds something to say every `chatterPeriodTicks`; a line already up runs its course before another may start, so the bubble never flickers between two things in consecutive ticks.

**Which line comes up and when is `Pet`'s decision rather than the scene's**, for the reason everything else about a companion is: a renderer holding a countdown, or picking a line from a generator of its own, is state a replay cannot regenerate.
The idle line is a hash of the tick it comes up on -- the murmur3 finalizer over exact-width integers, so it is the same line on every toolchain.
That is deliberately *not* a draw from an `antwika::rng` generator: a generator here would be a seed and a stream position for a save file to carry and keep in step with the ticks, and one let out of step would say the wrong thing for the rest of the session, where a hash of a number `Pet` already holds is nothing at all to carry.
It is deliberately not a plain `(tick / period) % count` either, which would be the same four lines in the same order forever and read as a carousel rather than as chatter.

The snapshot carries the `Saying` and not the words, and not the countdown: the words are the scene's table, and a scene handed a countdown could start counting one down itself.
That table is one table in one place, in `PetScene.cpp` beside the readout's words.
`antwika::i18n` is deliberately not used for it -- the readout beside the bubble is English written into that same file, so a catalogue holding one and not the other would translate the window by halves and leave two places to add a line to; moving both is a change worth making on its own.

The bubble sits left of the animal, under the two gauges and over the bowl, so it covers nothing that says anything, and it is drawn after the animal -- a bubble the companion stands in front of is somebody else talking.
Its text is scaled to the longest line the table holds rather than to the line being said, so the bubble is one size and the words one height throughout.
A window smaller than the one `main.cpp` asks for cannot give the smallest glyphs that much room and the longest lines overhang their bubble there, which is exactly where the readout already overhangs the grid; neither is clamped, so both stay one arithmetic rule.

### The "new pet" button

A perished companion is drawn with its grave, its own grey palette and one button reading `new pet`, painted into `reviveButtonRect()`'s box -- left of the grave, under the gauges and above the readout, so it covers nothing that says anything.
Whether it is drawn at all is the one thing the snapshot already says, so no renderer holds a note of a button being offered.
Pressing it calls `Pet::revive()`, which is the constructor's own idea of a new companion rather than a second list of starting values: no meals, no disturbances, no pesterings, no ticks and the balance this build ships, so a new companion never wears the last one's history.

`Pet::revive()` is legal at any time rather than only on a perished companion, deliberately.
Which press means "start again" is `ReviveSink`'s decision, made against the button it hit-tests, and a rule enforced in two places is one that can be enforced differently in each.

### The idle animation

The companion breathes, blinks and puffs in its sleep, and all three come from `antwika::animation`: a `Clip` is a definition and `resolve(clip, elapsedTicks)` is a pure function of the tick count the snapshot carries.
There is no animator anything advances, so drawing a frame cannot be a way for the picture to acquire state a replay would have to reproduce.
Which is also why the tick count is in the snapshot at all: it is the one number the picture moves from.

The breath is the one of the three that does not just pick a pose.
`resolve()` reports how far into its keyframe the tick is, and `kBreatheEasing` in `PetScene.cpp` tweens the two grid rows that frame sits between, through [`tween`](../libraries/tween.md).
It used to step: four poses a whole unit apart is eight pixels of jolt, twice a breath.
The curve is `QuadInOut` rather than linear, which would read mechanical, and rather than a sine, which is what a breath actually wants and is not computable exactly — see that page for why an inexact curve is not on offer.
The lift is therefore the one measurement in this scene in pixels rather than in whole grid units, which is what `raised()` exists to say.

### Why the pacer is a sink

A session has no end of its own -- it runs until the window is closed, or until a replay dispatches `engine.stop` -- so an unpaced one would spend a whole day of companion time in a fraction of a second.
The waiting is `simulation::TickPacer`'s, which is the one pacer this project has; `companion::PacingSink` is only the `ITickEventSink` shape around it.
That class is an `ecs::ISystem` because the two applications that reach for it keep their state in a `World` and register it as an observer, and this one keeps its state in a plain value.
The only thing between the two is the `World` in the signature, which `TickPacer`'s own documentation says it neither reads nor writes, so an empty one is the whole adapter -- and the alternative was a third copy of a class the project has already deduplicated twice.

## The companion between sessions

A live session carries on from where the last one left off.
`companion::PetMemory` is everything the simulation holds -- the tick count, the state, the hunger, the happiness, the three counts, the disturbed note and both speech-bubble fields -- taken out with `Pet::remember()` and put back in through `Pet`'s restoring constructor, so a round trip through the two is the identity.
It carries no configuration: which numbers a companion is balanced with is this build's decision and not a file's, since a saved one could quietly widen a day or hand a companion twice the happiness it can hold.

The file is a versioned JSON document read as `parse -> read version -> migrate -> validate -> decode`, exactly as `apps/game`'s save is, and for the same reason -- see [`docs/schema-versioning.md`](../../docs/schema-versioning.md).

- `kSaveMagic` is `antwika-companion`, checked first, so a replay or a game save handed to this loader is refused as the wrong kind of file rather than as a companion with every member missing.
- `kSaveFormatVersion` is 1, stated in `antwika::replay::kSchemaVersionKey` -- `"version"`, the one member every persisted document in this code base carries its version in.
  **A bump means a document written by an older build no longer satisfies this build's schema, or satisfies it and means something else**: renaming a member, requiring one that was optional, narrowing what a number may be, or reinterpreting a value.
  Adding an optional member is not that and needs no bump.
  A bump takes an `IMigration` from N to N+1 added to `standardPetMigrations()` and a test that loads a hand-written version-N document.
- `standardPetMigrations()` is this format's own `antwika::replay::MigrationChain`, empty today, constructed and injected rather than registered anywhere -- which is the whole reason that class is generic over an `nlohmann::json` and a version key.
- The state and the speech-bubble line are written as names (`asleep`, `zzz`) rather than as enumerator numbers, so reordering either enumeration cannot change what a file means, and the file stays hand-editable.

`companion::IPetStore` is the one seam to a filesystem, in `atlas_editor::IAtlasStore`'s shape and for its reason: every other class here is exercised with no file on disk, because a test hands the session a store that answers from memory and the session cannot tell the difference.
`FilePetStore` is the one implementation, and `PetSave.hpp` is the format -- split apart so a round trip through the format is assertable with no filesystem at all.

Three rules cover the awkward cases, and each is a decision rather than an accident.

- **When it is written: once, after the loop has finished.** Not every tick, which would be twenty writes a second of a file nothing reads in between, and not on a timer, which would be a clock inside a session that has none.
  The cost is stated rather than hidden: a session killed with `Ctrl+C` never reaches the epilogue and so keeps nothing, exactly as a `--record` run there writes no file.
  Closing the window is the way to end a session and keep it.
  A session that threw its way out of the loop keeps nothing either, since what it would keep is whatever state the failure left.
- **A replay neither loads nor saves.** A replay reproduces the session it recorded, and a companion loaded from whatever happens to be on the machine running it is a different starting state and so a different session -- the same file would replay to one thing here and another there, silently.
  Saving is refused for the mirror-image reason: a replayed session would overwrite the live companion with one regenerated from a recording.
  `storeIfLive()` is where that decision is made, so no `main()` has to remember it -- and an app's `main()` may hold no branch of its own anyway.
- **A file that will not read does not take the session with it.** A file that is *not there* is an ordinary "no previous companion, start a new one" and is not an error at all.
  A file that is there and will not read -- bad JSON, another format's magic, a version from a newer build, a member missing, or a set of numbers no live companion could be in -- is reported as a warning and a new companion starts, and the session goes on to write over it.
  Keeping the unreadable file would be kinder to whoever wants to look at it and would leave the application never able to save again, which is worse.
  A file that will not *write* is thrown as a `companion::SaveFormatError`, since the session is already over and the one thing left to say is that it was not kept.

`SaveFormatError` is its own type rather than `CompanionError`, per the one-exception-type-per-failure-category rule: a companion balanced on numbers no session could run on is a mistake in this build, where a file that will not read is a mistake somewhere else entirely.

## Running it

```sh
build/bin/antwika_companion/antwika_companion
build/bin/antwika_companion/antwika_companion --record demo.replay
build/bin/antwika_companion/antwika_companion \
  --replay build/bin/antwika_companion/demo.json
```

The shipped `demo.json` is a 38-second session with three well-timed meals and one rude awakening in it, and no pestering at all, so it plays out exactly as it did before that third violation existed; it ends with an `engine.stop`, so it finishes on its own.
A headless build reports neither a window close nor any input, so `Ctrl+C` is what ends a live run there -- and a `--record` run only writes its file once the run ends.
