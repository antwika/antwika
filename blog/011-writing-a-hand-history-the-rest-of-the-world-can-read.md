# Writing a hand history the rest of the world can read

*Post 11*

The [previous post](010-a-poker-hand-in-one-number.md) built `antwika::holdem` and `apps/poker` around one number for a hand's strength.
What it left behind was a narrator that only had to satisfy me: `TablePrinter` wrote lines like `alice raise to 30 (pot 75)` and `-- showdown, board 2c 3d 4h 7s 9c, pot 90`.
Readable, but its own dialect.
Poker has had a de facto text format for two decades — the one every hand-history tracker, replayer and equity tool already parses — and a table that talks in it costs nothing extra to build and makes everything downstream free.
This post is about what that format asked for that the code could not yet answer.

## The format is a transcript, not a log

The layout is a header naming the hand and the stakes, the seats and the stacks they started with, one line per posted blind, `*** HOLE CARDS ***`, one line per decision, a `*** STREET ***` marker as each card comes out, `*** SHOW DOWN ***`, who collected what, and a `*** SUMMARY ***` closing the hand off.
The important part is that every line is phrased from the outside: not "here is the state after the action", which is what a log wants to print, but "here is what a player at the table saw happen".

Two lines make that difference concrete.

`alice: raises 20 to 30` names both the size of the raise and the stake it raised to.
`Action` only carries the latter, deliberately — ["raise to 200" is unambiguous where "raise 200" is not](010-a-poker-hand-in-one-number.md) — and by the time the printer sees the action, `Table` has already replaced the bet it was measured against.

`bob: calls 90 and is all-in` names what the call actually cost.
A `Call` carries no amount at all, because the table works it out; and the obvious reconstruction — the difference between the stack before and after — is wrong precisely when the hand ends on that call, since the payout has already landed in the stack by the time anyone gets to look.

## Three fields, not a second copy of the betting rules

The printer could have kept its own running tally of every seat's round stake, the current bet and every stack, and derived all of it.
That is a second implementation of the betting arithmetic living in the presentation layer, drifting from the first one the moment either changes.

`StepOutcome` already documents itself as saying what a step did "in enough detail to narrate it without reaching back into the table", so the honest fix was to make that true: `staked` (what the action actually moved into the pot), `betBefore` (the round's largest stake before it), and `allIn`.
`TableRunner` fills them from the view it already builds for the agent plus one read afterwards, and `Table` is untouched.
`staked` is measured against the seat's `committed` rather than its stack, for the payout reason above — a hand that ends on the action has already paid itself out, but nothing ever adds to `committed` except the action itself.

## The uncalled bet nobody had to return

`Uncalled bet (200) returned to alice` is the one line the format expects that the library has no concept of.
It never needed one: `finishWithoutShowdown()` is a showdown with every hand value left at zero, so the top layer of the pot — the part only one player was ever eligible for — comes back through the payouts with everything else, and no code path exists to hook a "return" onto.

It turns out not to need one now either.
An uncalled bet is just the top stake of a betting round when only one seat holds it, minus the next stake down, and the printer is already tracking each seat's round stake to write the action lines.
The subtraction is four lines and needs nothing from the pot code.

It is also, conveniently, only ever reachable on the step that ends the hand.
A round closes when everybody still holding chips has matched the bet, so a stake standing alone above the rest means everyone else is folded or all-in — which leaves fewer than two players able to wager, which is exactly the condition under which `Table` runs the board out and settles there and then.
So the printer computes it once, at the end, rather than after every round.

The same subtraction fixes the numbers around it.
`Total pot` and `collected` in this format are net of the returned bet, while `HandResult::pot` and `Payout::amount` are gross — and a payout that is *nothing but* a returned bet, which is what a losing all-in raiser gets back, drops its `collected` line entirely instead of printing `alice collected 0 from pot`.

## The hand that was over before it started

`apps/poker` has one case where nobody is ever asked anything: two blinds that were all the chips their owners had.
`StepOutcome::prompted` exists to distinguish it, and the old printer used it to skip straight to the result — no action happened, so there was nothing to narrate.

In this format that is not an option, because a hand with no header is not a hand history.
So the deal gets written up on the same step that settles it, which means reading a hand start off a table that has already finished the hand: stacks with the payout added back out, hole cards (still there — only the *next* deal clears them), and dealt-in seats taken from the showdown entries rather than the `inHand` flags the payout cleared.
The blinds themselves come from `committed` rather than `roundCommitted`, because a hand that ran the board out has reset the round three times over before anybody reads it.

One deviation from the format survives on purpose: `Dealt to` is printed for every seat, not just the one player whose cards a real history would know.
There is no hero at a table of bots, and hiding cards that `RoomConfig`'s seed regenerates anyway would cost information for no privacy that exists.
