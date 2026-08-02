# Codified music and sound effects

**This is a plan, not code.**
No type, header or file named here exists yet.
It is a plan document in the sense [`CLAUDE.md`](../CLAUDE.md) means one, so it is deleted once the work it describes has shipped and whatever reasoning is worth keeping moves onto the wiki pages of the libraries it created.

It supersedes the "musical layer above this" section of [`wiki/libraries/sound.md`](../wiki/libraries/sound.md) on three points, named under "Where this departs from the existing sketch" below.
Everything that section says which is not contradicted here still stands, and it is the better reference for tempo maps, modulation and plugin hosting.

## The goal

**A game's music and its sound effects are source, not binary.**
A sound is a value in a header or a pattern expression under version control, diffable in a review and mergeable in a branch, rather than a `.wav` somebody exported from a tool nobody else has.

The tree is already there by accident rather than by design: a search for `*.wav`, `*.ogg`, `*.mp3` and `*.flac` under `src/` and `backends/` finds nothing, and [`sound_demo`](../wiki/apps/sound_demo.md) generates its tone in `demoTone()`.
So this work does not remove any asset.
It makes the thing the tree already does expressive enough to be worth doing on purpose.

## What Strudel actually is

Strudel is a JavaScript port of TidalCycles, and underneath the notation it is four ideas.

- **Time is an exact rational number of cycles**, never a float and never a tick count.
- **A pattern is a pure function from a span of time to the events inside it**, so `query(begin, end)` is the whole interface and there is no cursor to advance.
- **An event carries two spans**: `part` is the fragment the query saw, and `whole` is the extent the event would have had if nothing clipped it.
- **Randomness is a positional hash of the time being asked about**, so asking about cycle 400 directly gives the same answer as arriving there by playing.

Everything else is a front-end.
The mini-notation (`"bd*2 [~ sd] <cp hh>"`) parses into that value model, and the WebAudio output layer is one consumer of it.

## Why it fits this repository unusually well

Each of those four ideas is a rule this codebase already enforces somewhere else, for a reason it arrived at independently.

- A pure query rather than a cursor is why [`animation`](../wiki/libraries/animation.md) has no `Animator` you advance and why `tween::ease` is a pure function of its arguments.
- Exact rational time has a precedent in `animation::Progress`, an unreduced numerator and denominator pair, and in `tween`, which throws rather than rounding when a denominator will not fit.
- Positional randomness is already required outright by the determinism rule in [`CLAUDE.md`](../CLAUDE.md), and [`rng`](../wiki/libraries/rng.md) exists to serve it.
- A span-shaped query composes under transformation, which is the property the whole combinator algebra rests on.

There is a fifth fit that the existing sketch misses.
**A pattern algebra should know nothing about notes**, exactly as [`wfc`](../wiki/libraries/wfc.md) knows nothing about grids and [`pathfinding`](../wiki/libraries/pathfinding.md) knows nothing about tiles.
Strudel patterns carry arbitrary values, and a pattern library here that hard-codes a `NoteEvent` would be the one place in the tree where a general algorithm was fused to one domain.

## Three libraries, not one

The sketch on the sound wiki page describes a single musical layer.
Splitting it into three keeps each one's dependencies honest and lets the first ship on its own.

`antwika::pattern` is the algebra: rational cycles, spans, events, combinators.
It depends on [`rng`](../wiki/libraries/rng.md) and nothing else -- not `sound`, not `time`, not `event`.
It has no idea audio exists.

`antwika::synth` is the sound: oscillators, noise, envelopes, filters, a voice pool.
It depends on [`sound`](../wiki/libraries/sound.md) and `rng`.
It has no idea musical time exists.

`antwika::sequencer` is the seam: the tempo map, the tick-to-frame-to-cycle maps, and the window query that turns one into the other.
It is the only place the other two meet, and it is small.

## antwika::pattern

### The types

```cpp
// An exact position or duration in cycles, reduced, int64 over int64.
struct Cycle { std::int64_t numerator; std::int64_t denominator; };

// Half-open, always.  An empty or reversed span is a PatternError.
struct Span { Cycle begin; Cycle end; };

// `whole` is absent for a continuous signal, which has no onset.
struct Hap { std::optional<Span> whole; Span part; Controls value; };
```

`Controls` is the value a pattern carries, and it is a small sorted set of `(ParamId, ParamValue)` pairs rather than a `NoteEvent`.
`ParamId` is symbolic and caller-supplied exactly as `ui::WidgetId` is, so what crosses between subsystems is never declaration order.
`ParamValue` is fixed point rather than rational, which answers the open question the sound wiki page leaves: a modulated parameter is added to and scaled repeatedly, and a rational's denominator grows without bound under that, while a fixed-point value's representation does not.

Time stays rational and values go fixed point, and the split is deliberate.

### Patterns are values, not references

```cpp
class Pattern
{
public:
    void query(Span window, IHapSink &out) const;
    // ...
private:
    std::shared_ptr<const IPattern> impl;
};
```

The sketch on the wiki has each combinator hold a reference to its operand, which means every caller must keep every intermediate alive.
That forbids the thing Strudel is for, which is composing an expression:

```cpp
const auto lead = every(4, rev, fast({3, 2}, euclid(5, 16, arp)));
```

An immutable, shared-pointer-backed `Pattern` makes that safe, and the allocation is harmless because it happens where patterns are built, which is setup or the tick path, and never where samples are written.
**`query()` itself allocates nothing** -- it walks the graph and pushes into a caller-supplied sink, which is what lets a sequencer query into a scratch buffer it sized once.

`query()` is not `noexcept`, because rational arithmetic can overflow, and it does not need to be: it runs in the tick path and never on the render path.

### The combinators worth having first

Construction is `pure`, `silence`, `seq` (one cycle split between its arguments), `slowcat` (one argument per cycle) and `stack`.
Time is `fast`, `slow`, `early`, `late`, `rev`, `iter`, `every`, `off`, `compress` and `zoom`.
Structure is `euclid`, `euclidRot`, `struct`, `mask`, `degradeBy` and `sometimesBy`.
Value is `withValue`, `set`, `add`, `mul` and the named control helpers a caller actually writes, such as `note`, `gain` and `pan`.
Thickening is `superimpose` and `jux`.

`euclid` is Bjorklund's algorithm, which is integer, terminating and straightforward to cover completely.

### Randomness

`degradeBy(p, seed)` is `hash(seed, patternId, cycle) < threshold` and never a generator anything advances.
The mechanism reuses what exists: a `rng::SplitMix64Rng` is constructed from the mixed seed at the point being asked about, and one `next()` is drawn from it.
That is stateless in effect, which is what makes asking about cycle 400 out of order give the same answer as playing to it.

### Errors

`PatternError` for a reversed or empty span, a zero denominator, and an overflow that a reduced `int64` rational cannot hold.
The overflow path is the one to take seriously rather than the one to hope about, and `tween::ease` already shows the house answer: throw, do not saturate and do not silently round.

### Why this is testable to an unusual degree

A query into a vector-backed sink is one `EXPECT_EQ` against a literal list of events.

```cpp
EXPECT_EQ(hapsIn(euclid(3, 8, pure(kKick)), Span{{0, 1}, {1, 1}}), expected);
```

There is no mock, no device and no audio anywhere in that assertion, which is the same property that makes `ui::Frame::rects` the way a widget's position is checked.

## antwika::synth

### The payoff of live voices

A sound effect in the sfxr and ZZFX tradition is about twenty numbers: a waveshape, an attack, a sustain, a release, a frequency, a frequency slide, a sweep, a noise mix, a filter setting.
**That parameter set is exactly one synth voice whose frequency is modulated.**
A music note is also one synth voice.

So there is no separate sound-effect subsystem.
There is one voice pool, and an explosion and a bassline are two triggers into it, differing in their parameters and in what decided to fire them.
That unification is what makes generating voices rather than pre-rendering buffers the right call: the alternative bakes every effect into a `Waveform` at startup and then needs a second, live path the moment music has to run longer than memory allows, and the two paths drift.

For scale, a stereo minute at 48 kHz in normalised float is about 23 MB, so pre-rendering is survivable for effects and not for a soundtrack.

### The shape

`SynthMixer` implements `sound::IRenderCallback` and mirrors `sound::Mixer` deliberately, method for method.

- A fixed voice pool sized in the constructor and never resized.
- `trigger(const TriggerRequest &)` resolves everything and may throw, so nothing on the render path can fail or look anything up.
- `render(SampleBuffer, FrameIndex firstFrame) noexcept` allocates nothing.
- Round-robin voice stealing when the pool is full.
- `startFrame` is absolute and "now" is deliberately not expressible.

Because it is an `IRenderCallback`, every existing device, backend and conformance test takes it unchanged, and `sound` gains nothing at all.

The parts underneath are `Oscillator` as a pure function of phase over sine, saw, square, triangle and noise; `Adsr` as four frame counts and a sustain level driven by a branch-free stage machine; and `Svf`, a two-pole state-variable filter giving low-pass, high-pass and band-pass from one core.

### Noise is seeded per voice, never globally

A voice holds its own `rng::SplitMix64Rng`, seeded from the trigger's own parameters and its absolute start frame.
A single generator shared by the pool would make each voice's output depend on how many other voices were sounding and in what order they were stolen, which is a divergence that only appears under load.

### How it is tested without a device

`sound::OfflineDevice` already renders into a `Waveform`, so the whole of this is exercised with nothing open and no wall-clock time spent.

Assertions are on properties rather than on golden buffers: that an envelope's tail reaches silence, that a low-pass reduces the energy above its cutoff, that a stolen voice stops contributing, that a triggered voice starts on the frame it named and not on the buffer boundary.
Exact-buffer comparison is reserved for a pinned build, because the promise in [`CLAUDE.md`](../CLAUDE.md) is that Antwika's own samples are bit-exact for a fixed build and only practically identical across compilers.

## antwika::sequencer

### Three clocks, two exact maps

`time::Tick` is when decisions are made.
`sound::FrameIndex` is the one true clock everything is ultimately placed against.
`Cycle` is what music is written in.

Tick to frame is `floor(tick * numerator / denominator)` with frames-per-tick held as a rational, so the residue lives in the expression rather than in a running variable that can lose count.
48000 over 60 is 800 exactly, and 48000 over 59 alternates 813 and 814 forever without drifting.

Cycle to frame is a `TempoMap`: a sorted table of segments, each holding a start cycle, a start frame and a rate, looked up by binary search and one exact multiply.

**Neither map ever touches a float**, and that is the whole reason a note lands where the score says it does.

### The onset rule

Each tick, the sequencer advances a half-open lookahead window, converts it to a `Span`, queries the pattern, and triggers a voice for **every hap whose `part.begin` equals its `whole->begin`**.

That test is the single most important line in the library.
A hap without it is a fragment of an event the window happened to cut, and triggering on fragments retriggers every sounding note at every window boundary -- which is the classic way a Strudel port sounds like a machine gun.
A hap with no `whole` at all is a continuous signal and is read as a parameter value, never triggered.

The window is monotone and half-open, and its length is a whole number of ticks not smaller than the device's buffer, so no hap is emitted twice and none is skipped.

**How far ahead to query is a per-app number rather than a constant of this library.**
There is no standing tick rate in this tree: `simulation::TickPacer` takes an interval per app, and the apps that exist run at 40 ms, 50 ms and 80 ms.
A lookahead is therefore stated in ticks by whichever `main()` composed the sequencer, which is one more reason for the sequencer to be its own library rather than something folded into `synth`.

## What needs no new decision

These follow from rules already in [`CLAUDE.md`](../CLAUDE.md), and getting them wrong is the class of mistake that looks fine live and surfaces as a divergent replay far from its cause.

- **Nothing musical is ever recorded.**
  A pattern query is a pure function of a span and a trigger is *derived* from simulation state, so by the rule that only externally-supplied input is persisted, both are regenerated.
  No `music.*` or `sfx.*` event name may exist, for exactly the reason no `ui.*` one does.
- **Musical position is derived from `Tick`, never from `framesPlayed()`.**
  That remains legal only for deciding how long to sleep.
- **Oscillator phase, envelope stage and filter memory are projection state**, regenerable and in no snapshot, and nothing on the audio side flows back into a tick.
- **The note stream is the determinism promise, and the samples are not.**
  A test that needs a number asserts on the triggers, which are integers and fixed point through pure functions.
- **A note's placement does not depend on the tick rate, and a game-triggered effect's does.**
  A note's frame comes from the score through the `TempoMap`, so it is sample-accurate however coarsely the window is queried, and the tick decides only *when* the query happens rather than where anything lands.
  This is the whole return on the two maps being exact.
  An effect the game fires is different in kind, because the decision to fire it is itself made on a tick: it can be placed no earlier than that tick's frame, so it carries up to one whole interval of latency and jitter, which is 80 ms in [`tower_defence`](../wiki/apps/tower_defence.md) against a threshold nearer 10 ms.
  **That is accepted rather than worked around**, and it is written here so that a hit sounding late is recognised as this rather than chased as a bug.
  The only lever is the app's own tick interval, and raising one costs the retuning of every tick-denominated constant the app was tuned with -- `companion`'s drain rates, `life`'s generation speed -- and not the validity of its recordings, which store no tick rate and reproduce identical state whatever the pacing.

## Where this departs from the existing sketch

The plan on [`wiki/libraries/sound.md`](../wiki/libraries/sound.md) chose differently on three points.

**Rational cycles rather than integer pulses at 960 PPQN.**
960 divides cleanly by 2, 3, 4, 5 and 6, which is most of what music needs and not all of it: 960 over 7 is not an integer, so septuplets and deeply nested subdivisions quantise.
Rationals cost an overflow class of bug that pulses do not have, and the answer is `PatternError` rather than a wider integer.

**A sampler is not the primary instrument.**
The sketch opens with `instrument kick = sampler("kick.wav")`, which is the thing this plan exists to avoid.
A synthesised descriptor is the citizen, and a sampler is at most an afterthought.

**Patterns are values rather than references to operands the caller keeps alive.**

## Suggested order

**Stage one is `antwika::synth`**, and it is useful the day it lands: oscillators, envelope, filter, voice pool, seeded noise, and a handful of codified effect descriptors.
It needs no musical time, no patterns and no change to `sound`, and it discharges the sound-effect half of the goal completely.

**Stage two is `antwika::pattern`**, which needs no audio and is asserted entirely with `EXPECT_EQ`.

**Stage three is `antwika::sequencer`**, which is small once the other two exist, plus an app that actually plays something.

**Stage four, later, is the mini-notation front-end**, translating `"bd*2 [~ sd]"` into the value model, with one error type of its own.
It is a thin translation onto a fully tested algebra rather than a grammar the semantics live inside, which is the [`ui`](../wiki/libraries/ui.md) move and the reason for this order.

## The traps, named in advance

- **The onset rule above.**
  If nothing else in this document survives review, that does.
- **Denominator growth.**
  `Cycle` must reduce by `gcd` at every operation, because positions are compared by value -- which is the opposite of `animation::Progress`, which must *not* reduce, because it compares on the pair.
  That difference is easy to copy wrongly from a neighbouring library.
- **The coverage gate applies to all of this.**
  All three libraries live under `src/`, and `backends/` is the only exemption.
  So every envelope stage transition, every filter mode, voice-pool exhaustion and every rational overflow path needs a test that reaches it deliberately.
- **Window arithmetic.**
  Overlapping windows double-trigger and gapped ones drop notes, so half-open and monotone is not a stylistic preference here.

## Open questions

- **Where should the sequencer live?**
  Its own library is cleanest, but it is small, and folding it into `synth` costs `synth` a dependency on `pattern` and its ignorance of musical time.
- **Should a score be serialisable?**
  Nothing needs it while scores are compiled-in C++ values, and a JSON score would let a replay carry one inline rather than by path.
- **May any of this depend on [`ecs`](../wiki/libraries/ecs.md)?**
  The recommendation is no, mirroring [`ui`](../wiki/libraries/ui.md), though a voice pool is a very natural `World`.
