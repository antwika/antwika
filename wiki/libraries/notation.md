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

**Each `?` gets its own seed, counted left to right.**
Two of them in one string thin out differently, and both do so the same way on every run.

## See also

- [`pattern`](pattern.md) — the algebra this reads into.
- [`sequencer`](sequencer.md) — what usually plays the result.
