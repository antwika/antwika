# antwika::notation

`src/libs/notation/` — the mini-notation, read into the algebra.

## What it is for

Writing `"0 [3 5] <7 9>"` instead of `fastcat({pure(0), fastcat({pure(3), pure(5)}), slowcat({pure(7), pure(9)})})`.

It is the notation TidalCycles and Strudel are written in, over the patterns [`pattern`](pattern.md) already provides.

## The grammar

| Written | Means |
| --- | --- |
| `a b c` | one cycle split between them, in order |
| `~` | a rest |
| `[a b]` | a sequence occupying one slot |
| `[a, b]` | both at once |
| `<a b>` | one of them per cycle, in turn |
| `a*2` | twice as fast |
| `a/2` | half as fast |
| `a*3%2` | an exact ratio |
| `3%2` | a word a reader may read as a fraction |
| `a!3` | three slots of it |
| `a(3,8)` | its onsets on a Euclidean rhythm |
| `a?` | half its events dropped, reproducibly |

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `ParsePattern.hpp` | `parsePattern()` | The whole public surface. |
| `IWordReader.hpp` | `IWordReader` | What one word means. |
| `NumberWords.hpp` | `NumberWords` | Reads every word as a number under one `ParamId`. |
| `NotationError.hpp` | `NotationError` | This library's one failure type. |

## Depends on

[`pattern`](pattern.md), and nothing else.

## Non-obvious decisions

**It is a translation, not a second implementation.**
Every form in the table is spelled out in terms of a combinator that already existed and was already tested, which is why this library has one error type and no semantics of its own.
That is the whole reason the value model was built first: a grammar written before the algebra would have had the meaning of a pattern living inside a parser.

**The grammar knows what a word *is*; only an `IWordReader` knows what one *means*.**
`"0 3 5"` is three words, and whether they are scale degrees, sample numbers or filter cutoffs is the application's decision.
So this library stays as ignorant of music as [`pattern`](pattern.md) is, and `NumberWords` is a convenience rather than a commitment.

**Syntax errors are `NotationError`; everything else is `PatternError`.**
`"3(9,8)"` parses perfectly well and asks for nine onsets across eight steps, so the algebra refuses it rather than the grammar.
The split keeps the parser from having to know which combinations are meaningful.

**A speed is a whole number or a ratio written with `%`, and never a decimal.**
An exact ratio is the entire point of the time model underneath, and `3%2` preserves it where `1.5` would throw it away on the first character.
`%` is a word character too, so `3%2` is also how a fraction reaches a word reader -- and it cannot be mistaken for the `%` of a ratio, since a ratio is read digit by digit and never asks for a word.

**Every number is bounded at 1024, and so is the product of the speed factors along one nesting path.**
Without a bound, `"0!2000000000"` and `"0(3,2000000000)"` parse cleanly into a vector of two billion patterns, so the parse dies of `std::bad_alloc` rather than throwing, and `"0*999999999"` parses into a query window millions of cycles wide, which [`pattern`](pattern.md) walks one cycle at a time.
Either one defeats [`music_editor`](../apps/music_editor.md)'s whole resilience claim that a line which will not parse leaves the last one playing, since neither is something the editor can catch.
A thousand and twenty-four slots is finer than one cycle can be heard to articulate, so the limit refuses nothing musical.
The product is bounded separately because `*` composes: four factors of sixty-four nest into a factor of sixteen million in a dozen characters, and a per-factor bound alone would let that through.
Sequences and brackets are deliberately not counted towards it -- `"0*64 3*64"` is two independent terms, not a factor of four thousand.

**Each `?` gets its own seed, counted left to right.**
Two of them in one string thin out differently, and both do so the same way on every run.

## See also

- [`pattern`](pattern.md) — the algebra this reads into.
- [`sequencer`](sequencer.md) — what usually plays the result.
