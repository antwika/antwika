# antwika::pattern

`src/libs/pattern/` — a function from a stretch of time to the events in it.

## What it is for

**A pattern algebra over exact rational time**, in the shape TidalCycles and Strudel arrived at.

A pattern is a pure function: hand it a window and it tells you what is inside.
Combinators transform that function -- faster, slower, shifted, reversed, thinned, Euclidean -- and the set is closed, so an expression composes without special cases:

```cpp
const auto kick = pure(Controls(kNote, ParamValue(0)));
const auto riff = rev(fast(Cycle(3, 2), euclid(3, 8, kick)));
```

## What it is deliberately not

**It knows nothing about music.**
There is no note, no pitch, no bar and no tempo anywhere in it -- exactly as [`wfc`](wfc.md) knows nothing about grids and [`pathfinding`](pathfinding.md) knows nothing about tiles.

An event carries `Controls`, a set of `(ParamId, ParamValue)` pairs, and this library never interprets one.
A pattern of pitches, a pattern of filter cutoffs and a pattern of a pattern's own density are the same type here, differing only in an id the application gave meaning to.

It also knows nothing about audio: it does not depend on [`sound`](sound.md) or [`synth`](synth.md), and it cannot name a frame.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Cycle.hpp` | `Cycle` | A position or length in cycles, as an exact reduced fraction. |
| `Span.hpp` | `Span` | A half-open stretch, and the cycle-by-cycle split every combinator uses. |
| `ParamId.hpp` | `ParamId` | Names one thing an event carries; the caller chooses the values. |
| `ParamValue.hpp` | `ParamValue` | What a control holds, in fixed point. |
| `Controls.hpp` | `Control`, `Controls` | The value an event carries, sorted by id. |
| `Hap.hpp` | `Hap` | One event: a `part`, an optional `whole`, and a value. |
| `IHapSink.hpp` | `IHapSink` | Where a query hands what it found. |
| `HapBuffer.hpp` | `HapBuffer` | The sink that collects into a vector. |
| `Pattern.hpp` | `IPattern`, `Pattern` | The query interface, and the value that owns one. |
| `Patterns.hpp` | `silence`, `pure`, `steady`, `stack`, `slowcat`, `fastcat`, `timecat` | Building one. |
| `Combinators.hpp` | `fast`, `slow`, `early`, `late`, `rev`, `euclid`, `degradeBy`, `during` | Transforming one. |
| `PatternError.hpp` | `PatternError` | This library's one failure type. |

## Depends on

[`rng`](rng.md), for `degradeBy`, and nothing else.

## Non-obvious decisions

**Time is an exact rational, not a count of subdivisions.**
A fixed resolution -- 960 pulses to the quarter, say -- divides cleanly by two, three, four, five and six, and not by seven.
So it quantises a septuplet and every deeply nested subdivision, and composing four time transformations accumulates the error.
`FastcatSplitsIntoSeventhsExactly` is the test that pins this, and it is the reason the whole library is built on `Cycle` rather than on an integer.

The cost is that a denominator grows under composition and eventually will not fit, which is a `PatternError` rather than a rounding -- the same call [`tween`](tween.md) makes when an easing's denominator overflows.

**A `Cycle` compares by value, which is the opposite of `animation::Progress`.**
That type compares on the pair, because a clip run for four ticks did not run for two.
This one is a *position*: one half and two quarters are the same moment, and a combinator that arrived there by a different route has to agree that it did.
So a `Cycle` is always reduced, and the difference between the two types is easy to copy wrongly from a neighbouring library.

**Ordering walks rather than widens.**
Cross-multiplying two fractions is the obvious comparison and needs an integer twice as wide; `-Wpedantic` rightly refuses the `__int128` that GCC and Clang offer.
So `operator<=>` expands both sides as continued fractions instead, comparing whole parts and then the reciprocals of what is left, reversing the answer at each step.
It is exact, it cannot overflow, and it is `noexcept` -- which matters, because an ordering that could throw is one no container could use.

**A window, never a cursor.**
A cursor is stateful, so it cannot be shared between a sequencer and a test, cannot be asked out of order, and cannot be transformed without the transformation becoming stateful too.
A window composes: `fast` scales it, `early` shifts it, `rev` reflects it inside its own cycle, and each forwards the mapped window to its operand and maps every span that comes back through the inverse.
One class, `TimeMappedPattern`, is all four of `fast`, `slow`, `early` and `late`, because all four are the same scale-and-shift.

**A `Hap` carries two spans, and confusing them is the most expensive mistake here.**
`part` is what the query saw; `whole` is what the event would have covered had nothing clipped it.
A sequencer triggers on `hasOnset()` -- `whole` present *and* starting where `part` does -- and never on a hap merely existing.
Trigger on every hap and every sounding note restarts at every window boundary, which is how a port of this idea comes out sounding like a machine gun.

**A hap with no `whole` at all is a continuous value**, which is what `steady()` produces: it never begins, so it is read as a parameter and never triggered.
That is what automating a cutoff or a gain across a run is made of.

**Patterns are values, not references to operands a caller keeps alive.**
Every combinator takes patterns by value and returns one, over a `shared_ptr<const IPattern>`, so an expression owns everything inside it and nothing has a lifetime rule written in a comment.
The graph itself is allocated where a pattern is *built*; **`query()` returns no containers**, walking that graph and handing each event to the caller's sink instead.
That is not the same as allocating nothing: `pure`, `slowcat` and `rev` each split their window with `Span::spanCycles()`, which builds a fresh vector per query at every level it appears at.
A window a handful of cycles wide costs a handful of small vectors; one millions of cycles wide is why a speed factor has an upper bound in [`notation`](notation.md).

**`degradeBy` hashes its position rather than drawing from a generator.**
A generator advanced per event would make the answer depend on how many events had been asked for and in what order, which breaks replay the moment a lookahead window changes size.
Hashing means asking about cycle four hundred directly answers exactly as playing there would, and `DegradeAnswersTheSameHoweverItIsAskedFor` is the test that says so.

**`degradeBy` passes a continuous value through untouched**, for the same reason.
A hap with no `whole` has no onset to hash, and its `part` is only ever the window the caller asked about -- so thinning one at all would make whether a steady cutoff survives depend on how a caller sliced its queries, which is exactly the property above.
A signal has nothing to drop; `DegradeKeepsAContinuousValueHoweverItIsAskedFor` pins it.

**`timecat` is first-class because a composed one would owe fragments it could not pay.**
`fastcat` is one line -- `fast(n, slowcat(parts))` -- and a weighted sequence can almost be said the same way, with a long slice as a `slow` pattern followed by silences.
Almost: a window that saw only the middle of the held note would meet a silence slot and get nothing back, where the contract says a cut event comes back as a fragment -- its `part` inside its `whole`.
So `timecat` maps each slice's run of the cycle onto one cycle of its pattern and back, fragments included, and the cycle number rides through unchanged so an alternation inside a slice still turns per cycle, exactly as in a `fastcat`.

**`during` restarts its pattern at every window, and that is why it is not `slowcat`.**
`slowcat` hands a slot its pattern's *nth* cycle, so an alternation inside one advances once per revolution of the whole -- the right arithmetic for interleaving, and the wrong one for a song section.
`during(period, windows, p)` instead hands every window `p`'s own cycle zero, so each occurrence of a section sounds the same and a `slowcat` inside it advances once per cycle again.
An event cut by its window's edge keeps its `whole`, so a note that began inside may ring past the boundary while nothing new begins outside -- the same part/whole discipline as everywhere else.
The period is explicit rather than derived from the last window, because a schedule may end in silence.

**`euclid` uses the Bresenham formulation** -- a step sounds when `(step * pulses) % steps < pulses`.
It needs no working array and is exact integer arithmetic rather than a repeated subdivision.

**A `Span` that ends where it began is refused rather than treated as empty.**
Every combinator either forwards its window or maps it, so one that produced an empty span produced it from arithmetic that had already gone wrong.
Half-open everywhere is not a stylistic preference: two windows sharing an endpoint trigger whatever sits on it twice, and two leaving a gap drop it, and neither shows up in a test of one window.

## See also

- [`synth`](synth.md) — what a pattern's events eventually become audible through.
