# A poker hand in one number

*Post 10*

The [previous post](009-json-wins-tickevent-and-three-mains-that-stopped-repeating-themselves.md) retired the binary replay format and pulled three `main()`s back to the same shape.
This one adds a fourth application on that replay system, `apps/poker`, and the library underneath it, `antwika::holdem` — the same "library owns a mechanism, an app demonstrates it end to end" split `antwika::scheduler`/`apps/task_worker` established, applied to no-limit Texas hold'em.

## The requirement that shaped everything else

The brief asked for hand evaluation done with bitwise operations, and for a hand to be **one number**: greater is a stronger hand, and equal means the two hands are the same value in poker.

That second half is the stronger constraint, and it is worth being precise about what it rules out.
It says there is no secondary tie-break — no "compare categories, then compare kickers, then compare suits" helper sitting anywhere.
Two players split a pot if and only if their numbers match, so every detail poker cares about has to already be inside the integer, and every detail poker *doesn't* care about has to be absent from it.

The layout that satisfies both directions is a category in the high bits above five 4-bit rank slots:

```
 category │ slot 0 │ slot 1 │ slot 2 │ slot 3 │ slot 4
   4 bits │ 4 bits │ 4 bits │ 4 bits │ 4 bits │ 4 bits
```

Slots are filled most-significant first with the ranks that matter *for that category*: the trips rank then the pair rank for a full house, the pair rank then three kickers for one pair, just the top card for a straight.
Unused slots are left zero, which is not a placeholder so much as the point — a category that stops looking after two ranks has every one of its hands agreeing on the remaining three, so they compare equal, which is exactly the poker answer.

## Counting without counters

With the output shape fixed, evaluation is a question of getting the ranks into those slots.
A `Card` is packed as `(rank << 2) | suit`, which means a hand can be reduced to four 13-bit rank masks — one per suit — in a single pass with no lookup table.

From there, nothing needs counting.
Every rank held by at least *n* of the four suits is the union of the *n*-way intersections of those masks:

```cpp
const auto twiceOver = static_cast<std::uint16_t>(
    (clubs & diamonds) | (clubs & hearts) | (clubs & spades)
    | (diamonds & hearts) | (diamonds & spades) | (hearts & spades));
const auto quads = static_cast<std::uint16_t>(
    clubs & diamonds & hearts & spades);
```

Pairs are `twiceOver & ~thriceOver`, trips are `thriceOver & ~quads`, and the whole business of "how many nines are there" never happens.

Straights are the part that usually grows a special case, because the ace plays both high and low.
It doesn't need one:

```cpp
const std::uint32_t aceLow = (rankMask >> rawValue(Rank::Ace)) & 1U;
const std::uint32_t extended =
    (static_cast<std::uint32_t>(rankMask) << 1U) | aceLow;
const std::uint32_t runs = extended & (extended >> 1U) & (extended >> 2U)
                           & (extended >> 3U) & (extended >> 4U);
```

Shifting the mask up one bit and putting the ace back in at bit 0 means the 5-4-3-2-A wheel is found by the same five-way shift-and as every other straight, and lands below every other straight in the ordering for free.

Because none of this enumerates anything, there is no five-card subset picking step at all, and scoring seven cards costs the same as scoring five.

## The loop, and where the tick went

The brief also asked for a loop that prompts each agent for an action, over pre-flop, flop, turn, river and showdown, starting a new hand whenever one finishes.

The engine already has a loop — a fixed tick — so the question was whether poker should own a second one.
It shouldn't, and the resolution is that **one engine tick is one step of the poker loop**: either a hand is dealt, or exactly one player is asked what they want to do.
`TableRunner::step()` does at most one of those and returns a `StepOutcome` describing which.

That split keeps `Table` free of any opinion about who is playing.
`Table` answers "what is legal and whose turn is it"; `TableRunner` answers "whose opinion do we need next"; `IAgent` answers "what do I want to do".
A test can drive the state machine directly through `apply()` without an agent existing at all, which is how the betting rules get tested — and how a rules bug found while writing those tests could be pinned down precisely.

A hand also advances itself as far as it can between decisions.
Dealing the next street, running the board out when nobody has chips left to wager, and paying out at the end all happen inside `apply()`, which buys a useful invariant: while `isHandInProgress()` holds, `seatToAct()` always names somebody.
There is exactly one case where a hand finishes with nobody ever having been asked anything — two blinds that were all the chips their owners had — and that is what `StepOutcome::prompted` exists to distinguish.

## The rule the tests found

Writing the short-all-in tests turned up a genuine gap in the first implementation.

An all-in that falls short of a full raise doesn't reopen the betting.
The version I had written got half of that right: it left `actedThisRound` alone, so nobody who had already matched the previous bet was handed a fresh turn purely by the short all-in.
What it missed is that a player who *does* still owe the difference — and so has to be asked again — may only call or fold at that point, not re-raise.

That needed a second per-seat flag rather than a cleverer reading of the first:

```cpp
for (auto &other : seats)
{
    if (other.actedThisRound)
    {
        other.mayRaise = false;
    }
}
```

`TableView::mayRaise` surfaces it, so an agent can see that raising is off the table instead of proposing one and being rejected.
The test that pins it down walks a three-handed hand where the button limps, a short small blind shoves for four more than the big blind, the big blind (who had not spoken yet, and so keeps a full turn) calls, and the button is then asked for the difference and refused a raise.

## Side pots pay for themselves

Pot splitting is the other place where a general mechanism replaced several special cases.

`buildSidePots()` carves a hand's total commitments into a layer at each distinct amount an eligible seat put in, and `distributePots()` awards each layer to the strongest hand among its own contenders.
Two things fall out of that which would otherwise have been separate code paths.

An uncalled bet comes back on its own: if one player bets 100 and everyone folds, the top layer has no other eligible seat, so the split hands them their own chips back.
`finishWithoutShowdown()` is therefore the same call as a showdown with every hand value left at zero — there is no "return the uncalled portion" step anywhere.

And chips a folded player put in above what any remaining player could cover have nowhere lower to sit, so they ride along with the top layer rather than going missing.
Odd chips that don't divide between tied winners go to whoever is nearest the left of the button, which is why `distributePots()` takes a payout order rather than working in seat order.

## Money, and the one door between it and a seat

The last requirement was that agents track a balance outside the individual games, and can buy in without exceeding it.

`BankrollLedger` holds balances and knows nothing about tables, seats or hands.
`CashGame` is the only door between a bankroll and a stack: a buy-in withdraws exactly what it puts in front of a seat, a cash-out returns exactly what is left there.
Because those are the only two doors, the total of every bankroll plus every stack plus the pot is conserved across a whole session — which the tests assert directly, since it is a cheaper and stronger statement than checking any individual pot.

Everything that could refuse a buy-in is checked *before* the ledger is touched, so a refusal never strands chips between the two.
That ordering is the whole reason `buyIn()` looks for a free seat before withdrawing rather than after.

## What a replay of a poker session actually contains

The interesting consequence of building this on the tick loop is how little a recording holds.

Running the bundled demo produces 31 hands of poker and a replay file with **nine events**: four deposits, four buy-ins, and the stop.
Not one card, not one action.

That works because everything else is regenerated rather than recorded.
The shuffle is a hand-written Fisher-Yates over `SplitMix64Rng` — deliberately not `<random>`, whose distributions are not portable — seeded from `RoomConfig`, and `PolicyAgent` is a pure function of the `TableView` it is handed.
So the same events and the same seed deal the same cards to the same players and get the same decisions back, by construction rather than by convention, which is the same argument [post 1](001-building-a-deterministic-replay-system.md) made about `engine.tick` never belonging in a replay in the first place.

The agent has one property that matters more than how well it plays: it never returns an action the table would reject.
Every wager it names is clamped into `[minRaiseTo, maxRaiseTo]` and it checks `mayRaise` before offering a raise at all, so a short stack that cannot make the minimum raise asks to go all-in for exactly what it has.
That is asserted as a property, not case by case — a few hundred hands of real poker across four seats and three styles, which sweeps enough positions, stack sizes and short all-ins to be worth a single test.

## Coverage

The GNU leg stayed at 100% line/function/branch.
Most of the initial misses were real gaps and got tests: the defaulted `operator==` short-circuits on `Blinds`, `Payout` and `RoomSummary`, an occupied seat that isn't in the hand, and an action type that is none of the five — which used to be a silent no-op and is now an `IllegalActionError`, since a table that quietly ignored it would skip somebody's turn.

Three `switch` statements had unreachable implicit defaults and were better off without them.
`madeHandStrength` became a table indexed by `HandCategory`, `thresholdsFor` became two `if`s, and `TablePrinter::printStep` became an `if` chain — all three now have only branches a valid input can reach.

Only the cases matching [`docs/confirming-unreachable-branches.md`](../docs/confirming-unreachable-branches.md) were excluded: four exception-cleanup landing pads at closing braces holding a local `std::vector`, and two aggregate-initialisation lines whose only uncovered edges are the unwind paths of allocating a member.
`toString(Card)` avoided its landing pad outright by building the two-character string in one expression instead of pushing onto a named local.
