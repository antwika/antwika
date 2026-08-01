# apps/companion

`src/apps/companion/` — a tamagotchi in a 256-pixel window, whose energy is its life.

## What it is

`apps/companion` is an animal you keep alive by spending it.
Energy is the meter it lives on, happiness is the rate that meter runs down at, and sleep is the only thing that fills it back up.
But a companion is refused its bed until it has spent enough of the day to have earned one.
What follows is the rules and the numbers, and why they are those numbers.

## The loop

```
        play ──burns──┐
                      ▼
hunger ──┐        ┌────────┐        ┌─────────┐
boredom ─┼─drain──▶ ENERGY │◀─fill──│  sleep  │
         │        └────────┘        └─────────┘
         │             ▲                  ▲
         └─▶ happiness ┘             must be tired
             (sets the base rate)     to be allowed in
```

- **Energy drains while it is awake.**
  The base period comes from the band its happiness falls in.
  A famished companion and a bored one each pay a second drain on top of it.
- **Energy reaching nothing is a collapse rather than an end.**
  The companion drops where it stands, falls asleep, and loses `collapsePenalty` off its **energy ceiling** for good.
- **When the ceiling itself runs out, it has perished.**
  That is the only way one ever does.
- **Sleep ends when the energy is back**, rather than at an hour.
  Put to bed nearly empty it sleeps a long night; sent to bed early it wakes early, into a long day it has to last.
- **Bedtime is refused above `tiredPercent` of the ceiling.**
  Recovery cannot be banked: a companion has to spend the day near the edge before it is allowed to recover from it.

The skill this asks for is one sentence.
**Burn as close to nothing as you dare, then put it to bed before it drops.**
Playing is what spends the energy, feeding is what buys the room to play, and every collapse costs capacity that is never given back.
So a badly kept companion visibly frays rather than dying of one mistake.

**There is exactly one death condition, and it is the ceiling running out.**
Starving, boredom and misery kill nothing directly; all three only bring a collapse forward.
That is the rule this application always had -- every violation meets in one place -- read out over a resource somebody can watch shrink.

## The three verbs, and the one refusal

Three props stand on the ground, and a press means whichever one it landed on.
`companion::PetLayout.hpp` names their boxes, `propAt()` hit-tests them, and `PetScene` paints into the very same rectangles.
So what somebody aims at and what they hit cannot drift apart.

| prop | press | when it is right | when it is a slight |
|---|---|---|---|
| **bowl** | feed | hunger has reached the threshold | it is not hungry |
| **ball** | play | it has the energy to spend | it has less than `playEnergy` left |
| **nest** | bedtime | energy is at or under the tired mark | it is still wide awake |

- **A press on none of them is a prod**, and costs `pesterCost` like any other refusal.
  Sloppy aim has a price, which is what leaves the props worth aiming at.
- **A press of any kind while it is asleep wakes it up.**
  That costs `disturbCost` happiness and, far worse, ends the night with the energy it had rather than the energy it needed.
  That rule lives in `Pet` rather than in the sink: the sink says where a press landed, and the companion says what it was in a state to do about it.
- A press after it has perished means nothing at all.
  Unless it lands on the "new pet" button, which is the one press whose meaning depends on where it landed.

**`*yawn*` is a required part of the design rather than decoration.**
Bedtime is refused above the tired mark, so a player with no way of knowing when that window opened would be playing against a hidden rule rather than a hard one.
The companion yawns on the tick it crosses the mark, into an empty bubble so it never cuts another line off.

## Growing up, and what it grows into

How grown up a companion is is `stageAt()` over its tick count and nothing else: `Egg`, `Child`, `Teen`, `Adult`, `Elder`.
**What a stage decides is how much energy it may hold, and that is all.**
Growing up is capacity here, so the gauge and the picture tell one story with no number invented for it.

The ceiling is not stored anywhere.
It is `baseEnergyFor(stage)` less `collapsePenalty` per collapse, floored at nothing.
So a companion's remaining life is arithmetic over two numbers it already keeps -- the ticks it has lived and the times it has dropped -- and no third number can be put out of step with them.

**An elder holds a child's ceiling rather than an adult's**, which is the one stage that takes room back.
So old age can be what finally ends a companion that has collapsed often, with no further collapse needed.
`Pet::step()` checks for a ceiling of nothing before anything else, which is where that lands.

`companion::PetForm` is what it grew into -- `Bright`, `Plain` or `Scruffy` -- decided by `formFor()` over the care record and nothing else.
A collapse counts for three of anything else, since it is the one violation that takes something back.
The form reaches the picture as a shade on the one fur each palette holds, rather than three more colours in every palette.

## The day, and what kind of day it is

There is no clock here at all any more.
A companion is awake or asleep because it was put to bed, because it dropped, or because it woke.
**That is a real cost, stated rather than hidden.**
Version 1 of this application had `night(t) = (t % (dayTicks + nightTicks)) >= dayTicks`, which could not drift from anything, and coupling sleep to energy spends that.

A day ends when the companion wakes, and `day()` counts them.
`moodOn(day)` gives each day a `DayMood`, and each mood hurries exactly one period along by a third.

- `Hungry` shortens `hungerPeriodTicks`.
- `Restless` shortens `funDecayPeriodTicks`.
- `Heavy` shortens whichever drain period the happiness band picked.
- `Ordinary` changes nothing, and half of all days are one.

Which mood a day has is a hash of the day's own number, in the idle chatter's sense and for its reason.
A generator would be a seed and a stream position for a save file to carry and keep in step, where a hash of a number the companion already holds is nothing at all to carry.
**Day zero is deliberately `Ordinary`**, which is what keeps every trace in `PetTest` readable: nothing is hurried until a night has gone by.

## The numbers

Every period is written as a number of seconds times `kTicksPerSecond`, rather than as a tick count somebody has to divide in their head.
So changing how fast a companion lives is one constant, and every need keeps the same relationship to every other one.
`kTickInterval` -- what `PacingSink` waits, and the only place the wall clock appears at all -- is derived from the same constant.

- `kTicksPerSecond` is 20, so a tick is 50 ms.
- `kEnergyBase` is 30 and `kStageEnergyBonus` is 10, so an egg holds 30, an adult 60 and an elder 40.
- `kCollapsePenalty` is 10, so a whole life is a handful of collapses and no more.
- `kTiredPercent` is 40, so 60% of the ceiling has to be spent before a bed is allowed.
- `kRecoverPeriodTicks` is a quarter of a second, so a full night is shorter than a full day.
- The drain bands are 12, 9, 6 and 4 ticks, at happiness 8 and up, 4 and up, 1 and up, and none.
  A miserable companion therefore burns three times as fast as a happy one, which is the whole of what happiness is for.
- `kStarvePeriodTicks` is 3 seconds and `kFretPeriodTicks` is 4.
  Each costs **one happiness and one energy on the same beat** -- two rules with four effects rather than four rules.
- `kHungerPeriodTicks` is 2 seconds, `kHungerMax` 8 and `kHungerThreshold` 4.
  So there is a whole 8 seconds of visible warning before neglect costs anything.
- `kFunStart` is 10 and it decays every 2 seconds, so a companion nobody plays with is bored in 20.
- `kPlayEnergy` is 6, `kPlayFun` 4, `kPlayHunger` 2 and `kPlayJoy` 1.
  Four games is most of an egg's day, which is the trade the whole day is arranged around.
- `kDisturbCost` is 2 and `kPesterCost` is 1, one price for all four refusals.
  They are the same mistake, which is asking for something the companion is not in a state to give.
- `kHappinessMax` is 10 and `kHappinessStart` 6, so attentive play climbs into the happy band and buys itself a longer day.
- `kSayingTicks` is 3 seconds and `kChatterPeriodTicks` 6, so the bubble is empty for as long as it is full.
- The stages arrive at 40, 100, 200 and 400 seconds.

The balance those numbers add up to is asserted rather than described.
`PetTest` runs the shipped configuration three ways over 600 seconds: nobody attending it, somebody feeding and playing and putting it to bed on cue, and somebody playing with it without pause.
The first has to run out of ceiling, the second has to be fine and collapse not once, and the third has to spend itself to death.
That last one is what makes playing a risk rather than a free good.

## How it is wired

The shape is `apps/tower_defence`'s rather than `apps/life`'s, because a companion is one plain value rather than a grid of entities.

- `PropSink` decodes `input.pointer_*`, asks `propAt()` which prop the press landed on, and calls the verb it names.
  **The app defines no event for feeding, playing, putting to bed or prodding it**, deliberately.
  A `--record` run persists the press, and which prop it hit and whether the companion was in a state to answer are worked out again on replay from the same press against the same canvas and the same tick count.
  Persisting the meal as well would feed it twice per press.
- `ReviveSink` decodes the same presses and starts a new companion when one lands on the button a perished one is offered.
  It is registered *after* `PropSink`, so one press can never be a prop and a revival both.
  **It defines no event either.**
- `PetSink` calls `Pet::step()` once per `engine.tick`, registered after both so a press is answered by the state the last tick ended with.
- `RenderSink` takes a `PetSnapshot` -- an immutable value the scene cannot write -- and hands it to `PetScene`.
- `PacingSink` is last, which makes the order present-then-wait.

Everything is laid out and hit-tested against the *configured* window size rather than the size a window reports.
That is for the reason `life::PointerToggleSink` gives about cells: a hit-test is a function of the layout, and a resized window would resolve a recorded press to a different answer.

### Why the animal is rectangles rather than an atlas

The house style is a hand-drawn PNG atlas addressed arithmetically (see [`game-texture-atlas.md`](game-texture-atlas.md)).
It is right when a picture has hundreds of distinct tiles and an artist who is not the programmer.
This window is 256 pixels square and the animal is ten boxes.
An atlas for it would be a checked-in binary, a second contract in `TileAtlas.hpp`'s shape, and a startup check that the file is the size the header says -- all to hold a picture that is shorter written down than described.
Drawing it from `IRenderer`'s rectangles also leaves the whole scene assertable against a mock renderer call by call, and `PetSceneTest` asserts exactly that.

Everything is laid out on a grid of `companion::kSceneUnits` whole units a side, centred in the canvas, which 256 pixels divides into exactly eight pixels each.
`main.cpp` derives its window size from that constant times a pixels-per-unit number rather than naming a pixel count that happens to divide by it.

### The gauges, the props and the readout

Four gauges take the top eight rows: energy, hunger, fun and happiness.
**The energy gauge is the one whose own end moves.**
Its track is drawn to the ceiling the companion still has, so a collapse shortens the bar itself and not merely what is in it.

The three props sit along the ground, painted into the very boxes `propAt()` hit-tests.
**The one the companion would like is lit rather than merely present.**
That is this application's whole answer to instructions: what to press next is on the screen.

Three lines of text stand under them.
The first says what it is doing -- `awake`, `awake, hungry`, `awake, woken`, `asleep` or `gone`.
The second says how old the day is, how grown up it is, and what kind of day it is: `d3 teen heavy`.
The third says what the file remembers behind it: `gen 2 best 900`.
Every character of it is read off the `PetSnapshot`, so the readout reports the run rather than adding to it.

### The speech bubble

A bubble appears beside the animal and goes away again after `sayingTicks`.
Four of its lines are idle chatter (`hello!`, `bored...`, `nice day`, `la la la`).
Eleven say something about the run: `feed me!`, `play!`, `yum yum!`, `wheee!`, `im full!`, `so tired`, `not tired`, `*yawn*`, `hey!`, `shhh!` and `zzz...`.

**Which line comes up and when is `Pet`'s decision rather than the scene's.**
That is for the reason everything else about a companion is: a renderer holding a countdown, or picking a line from a generator of its own, is state a replay cannot regenerate.
The idle line is a hash of the tick it comes up on -- the murmur3 finalizer over exact-width integers, so it is the same line on every toolchain.
The snapshot carries the `Saying` and not the words, and not the countdown.

**The words themselves come from [`i18n`](../libraries/i18n.md), and `Pet` never sees one.**
`PetScene` holds a `Translator` and turns a `Saying` into a `MessageId`; the state line, the day line and the lineage line are worded the same way.
That split is what keeps the active language out of the state a replay reproduces: `Pet` is integer throughout and reads no clock, no generator and no locale, so a session recorded in English replays identically in Swedish and only the pixels differ.
The bubble is scaled to the longest line *the catalogue in use* holds rather than to a character count written into the scene -- the longest Swedish line is half again the longest English one, so a count baked in would have been the English one.

### The "new pet" button

A perished companion is drawn with its grave, its own grey palette and one button reading `new pet`, painted into `reviveButtonRect()`'s box.
Pressing it calls `Pet::revive()`, which is the constructor's own idea of a new companion rather than a second list of starting values.
Its collapses are included in that, since a new companion born with somebody else's spent ceiling is one session wearing another's history.

`Pet::revive()` is legal at any time rather than only on a perished companion, deliberately.
Which press means "start again" is `ReviveSink`'s decision, made against the button it hit-tests, and a rule enforced in two places is one that can be enforced differently in each.

## The record the file keeps

`companion::Lineage` is what survives a companion: which one of the file's line is current, and the longest any of them lived.
**It is kept apart from `PetMemory` because `Pet::revive()` replaces the companion whole.**
Anything meant to outlive that cannot be one of its fields, and `Pet::remember()` stays the identity round trip it always was because none of it is in there.

The age is recorded in *ticks* rather than in days.
A day is no longer a fixed length, so a day count would reward short days rather than long lives.
`ReviveSink` offers the ending companion's age to the record before replacing it, and moves the generation on after.
That order matters, since afterwards there is nothing left to ask how long it lived.
The session's epilogue offers the same thing, so a companion nobody replaced still sets the mark it earned; `record()` keeps only the longest, so offering it twice is offering it once.

## The companion between sessions

A live session carries on from where the last one left off.
`companion::CompanionMemory` is what a file holds: a `PetMemory` and a `LineageMemory`.
`PetMemory` is everything the simulation holds, taken out with `Pet::remember()` and put back through `Pet`'s restoring constructor, so a round trip through the two is the identity.
It carries no configuration: which numbers a companion is balanced with is this build's decision and not a file's.
**Neither the ceiling nor the stage is in it**, deliberately.
Both are arithmetic over `ticks` and `collapses`, and a stored copy is a second statement of one fact that a hand-edited file could put out of step with the first.

The file is a versioned JSON document read as `parse -> read version -> migrate -> validate -> decode`, exactly as `apps/game`'s save is -- see [`docs/schema-versioning.md`](../../docs/schema-versioning.md).

- `kSaveMagic` is `antwika-companion`, checked first, so a replay or a game save handed to this loader is refused as the wrong kind of file.
- `kSaveFormatVersion` is 3.
  Version 1 held the companion that died of unhappiness.
  Version 2 is the same file describing an animal whose energy is its life: it gained `fun`, `energy`, `day`, `plays` and `collapses`, and `disturbed` became `woken`.
  Version 3 added `generation` and `bestTicks`.
- `standardPetMigrations()` holds both steps.
  **A version 1 companion keeps what still means the same thing** -- how long it lived, what was done to it, and whether it is still alive.
  The needs that have no reading here start where a new companion's start.
  A version 1 grave stays a grave: it arrives with enough collapses that its ceiling is certainly nothing, whatever age the document claimed.
- The state and the speech-bubble line are written as names (`asleep`, `zzz`) rather than as enumerator numbers, so reordering either enumeration cannot change what a file means.

`companion::IPetStore` is the one seam to a filesystem, in `atlas_editor::IAtlasStore`'s shape.
`FilePetStore` is its one implementation, and `PetSave.hpp` is the format alone.

Three rules cover the awkward cases, and each is a decision rather than an accident.

- **When it is written: once, after the loop has finished.**
  Not every tick, and not on a timer, which would be a clock inside a session that has none.
  A session killed with `Ctrl+C` therefore keeps nothing, exactly as a `--record` run there writes no file.
- **A replay neither loads nor saves.**
  A companion loaded from whatever happens to be on the machine running it is a different starting state and so a different session.
  `storeIfLive()` is where that decision is made, so no `main()` has to remember it.
- **A file that will not read does not take the session with it.**
  A file that is not there is an ordinary first run.
  A file that is there and will not read -- bad JSON, another format's magic, a version from a newer build, a companion no session could be in, or a lineage on its zeroth companion -- is reported as a warning and a new companion starts.
  A file that will not *write* is thrown as a `companion::SaveFormatError`.

## Running it

```sh
build/bin/antwika_companion/antwika_companion
build/bin/antwika_companion/antwika_companion --record demo.replay
build/bin/antwika_companion/antwika_companion \
  --replay build/bin/antwika_companion/demo.json
```

The shipped `demo.json` is a 35-second session of two well-played days: two meals, two games, two nights, and no collapse, no prod and no rude awakening between them.
It ends with an `engine.stop`, so it finishes on its own.
**It replaced the version 1 demo outright rather than being amended**, because a press now means whichever prop it landed on, and every recording written before this one lands somewhere else.
A headless build reports neither a window close nor any input, so `Ctrl+C` is what ends a live run there -- and a `--record` run only writes its file once the run ends.
