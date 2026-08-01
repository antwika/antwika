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
- A tap after it has perished is the only one that does nothing, since nothing about a perished companion ever changes again.

The three violations all spend the same currency, and what separates them is how much.
Letting it go famished costs one happiness every `starvePeriodTicks`; waking it costs `disturbCost` at once; pestering it costs `pesterCost` at once.
Happiness reaching zero is `PetState::Perished`, and there is no way back from it -- by any of the three.

## The day, the night, and where the clock lives

The day is a function of the tick count and of nothing else:

```
night(t)  =  (t % (dayTicks + nightTicks)) >= dayTicks
```

Falling asleep and waking up are therefore not countdowns that could get out of step with the day, and the sun does not stop for a companion that has perished.
Hunger grows only while it is awake, so a night is a night off from it.
An undisturbed night gives one happiness back every `restPeriodTicks`, and a disturbed one gives none -- the note of that is cleared on the *first tick of each night* rather than at dawn, so a tap in the daylight cannot spoil the night that follows it.

Nothing in `Pet` reads a clock, a locale or a generator.
It is a pure function of how many times `step()` has been called and of when `tap()` was called between them, which is the whole reason a recording of the taps replays to the same animal: not one byte of the hunger, the sleep or the happiness is persisted, and all of it is regenerated.

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

There is deliberately **no layout and no hit-testing**.
The window holds one animal and a press anywhere in it means the same thing, so unlike `life::BoardLayout` or `td::GridLayout` there is nothing that could let what somebody sees and what they can hit drift apart.
The scene is still laid out against the *configured* window size rather than the size a window reports, which costs nothing here and keeps the rule the same in every app a reader might open next.

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

### The idle animation

The companion breathes, blinks and puffs in its sleep, and all three come from `antwika::animation`: a `Clip` is a definition and `resolve(clip, elapsedTicks)` is a pure function of the tick count the snapshot carries.
There is no animator anything advances, so drawing a frame cannot be a way for the picture to acquire state a replay would have to reproduce.
Which is also why the tick count is in the snapshot at all: it is the one number the picture moves from.

### Why the pacer is a sink

A session has no end of its own -- it runs until the window is closed, or until a replay dispatches `engine.stop` -- so an unpaced one would spend a whole day of companion time in a fraction of a second.
The waiting is `simulation::TickPacer`'s, which is the one pacer this project has; `companion::PacingSink` is only the `ITickEventSink` shape around it.
That class is an `ecs::ISystem` because the two applications that reach for it keep their state in a `World` and register it as an observer, and this one keeps its state in a plain value.
The only thing between the two is the `World` in the signature, which `TickPacer`'s own documentation says it neither reads nor writes, so an empty one is the whole adapter -- and the alternative was a third copy of a class the project has already deduplicated twice.

## Running it

```sh
build/bin/antwika_companion/antwika_companion
build/bin/antwika_companion/antwika_companion --record demo.replay
build/bin/antwika_companion/antwika_companion \
  --replay build/bin/antwika_companion/demo.json
```

The shipped `demo.json` is a 38-second session with three well-timed meals and one rude awakening in it, and no pestering at all, so it plays out exactly as it did before that third violation existed; it ends with an `engine.stop`, so it finishes on its own.
A headless build reports neither a window close nor any input, so `Ctrl+C` is what ends a live run there -- and a `--record` run only writes its file once the run ends.
