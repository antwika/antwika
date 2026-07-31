# antwika::holdem

`src/libs/holdem/` — no-limit Texas hold'em rules.

## What it is for

The rules of a hold'em table: dealing, the betting rounds, stage progression, showdown and payouts.
It is a standalone domain library with no `antwika` dependencies; [`apps/poker`](../apps/poker.md) drives it from the tick loop, but nothing in the library knows that.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Table.hpp` | `Table` | Owns the betting rules, the stage progression and the side-pot payout. |
| `TableRunner.hpp` | `TableRunner` | Advances the table one step at a time; one step is one deal or one player being asked to act. |
| `StepOutcome.hpp` | `StepOutcome`, `StepKind` | What that step was, in enough detail to write a hand history from without recomputing the betting. |
| `TableView.hpp` | `TableView` | What a seat is allowed to see when asked to act. |
| `IAgent.hpp` | `IAgent` | Decides an `Action` from a `TableView`. |
| `Action.hpp`, `ActionType.hpp` | `Action`, `ActionType` | Fold, check, call, bet, raise. |
| `Stage.hpp` | `Stage` | Pre-flop, flop, turn, river, showdown. |
| `BettingRound.hpp`, `HandFlow.hpp` | `BettingRound`, `HandFlow` | One round of betting, and the progression through the stages. |
| `Card.hpp`, `Rank`, `Suit`, `CardText.hpp` | `Card` | A card as an `enum class : std::uint8_t`, with text parsing and printing. |
| `Deck.hpp`, `IDeck.hpp` | `Deck`, `IDeck` | The deck, behind an interface so a test can deal a fixed one. |
| `IRng.hpp`, `SplitMix64Rng.hpp` | `IRng`, `SplitMix64Rng` | The shuffle's randomness, seeded rather than sampled. |
| `HandEvaluator.hpp`, `HandValue.hpp`, `HandCategory.hpp` | `evaluate()`, `HandValue` | Scores 5–7 cards into one comparable number. |
| `SidePot.hpp`, `Payout.hpp`, `HandResult.hpp`, `ShowdownEntry.hpp` | — | How a pot is split and who won what. |
| `Seat.hpp`, `SeatId.hpp`, `Chips.hpp`, `Blinds.hpp`, `Limits.hpp` | — | Table furniture. |

Errors: `IllegalActionError`, `TableStateError`, `CardFormatError`, `DeckExhaustedError`, `HandEvaluationError`.
`FakeDeck` and `FakeRng` live under `tests/fakes/`, `MockAgent` under `tests/mocks/`.

## Depends on

Nothing.

## Non-obvious decisions

**A hand is one number.**
`evaluate()` produces a `HandValue` (an `enum class : std::uint32_t`) using only shifts, ands and ors over four per-suit 13-bit rank masks.
Greater is stronger and equal is a split pot, so comparing hands is `operator<` rather than a category-then-kicker cascade — and there is no tie-break code path to get wrong.
See [`blog/010-a-poker-hand-in-one-number.md`](../../blog/010-a-poker-hand-in-one-number.md).

**Randomness is injected and seeded.**
`IRng` and `SplitMix64Rng` mean the shuffle is a pure function of a seed, which is what lets `apps/poker` record only the deposits and buy-ins and regenerate every card.
The engine itself has no RNG at all; this one lives in the domain library where its seed is part of the recorded configuration.

**`StepOutcome` carries enough to reconstruct the narrative.**
The blinds, the raise sizes and the uncalled bet are derived from it rather than recomputed, which is what keeps `poker::TablePrinter` from being a second, drifting implementation of the betting rules.
