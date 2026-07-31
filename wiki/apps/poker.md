# apps/poker

`src/apps/poker/` — a no-limit Texas hold'em cash game.

## What it demonstrates

A whole domain library ([`holdem`](../libraries/holdem.md)) driven by the tick loop, with a replay that records only the money moving and regenerates every card and every decision.

## Running it

```sh
build/bin/antwika_poker/antwika_poker
build/bin/antwika_poker/antwika_poker --record demo.replay
build/bin/antwika_poker/antwika_poker --replay src/apps/poker/replays/demo.json
build/bin/antwika_poker/antwika_poker --tick-delay-ms 250
```

One engine tick is one step of the poker loop: a deal, or one player being asked to act.
It also opens a window and draws the table each tick.
`--tick-delay-ms <n>` holds each frame for `n` ms and keeps the final frame up until the window is closed; it defaults to 0, which is what keeps the default terminal run unchanged and stops the `null` backend (which never reports a close) from wedging it.
A real backend needs a display, so use `SDL_VIDEODRIVER=dummy` or `xvfb-run` without one.

## Libraries it composes

[`app`](../libraries/app.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`gfx`](../libraries/gfx.md), [`holdem`](../libraries/holdem.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), plus the selected graphics backend.
No ECS: the table *is* the state.

## How it is put together

- `poker.deposit`, `poker.buy_in` and `poker.cash_out` are the app's three event names, handled by `PokerRoomSink`.
- `BankrollLedger` tracks balances outside the games; `CashGame` is the only path between a bankroll and a seat.
- `PokerRoom` and `RoomConfig` own the table, its `holdem::TableRunner`, and the shuffle seed.
- `PolicyAgent` and `AgentStyle` implement `holdem::IAgent`.
- `TablePrinter` writes hand histories; `WatchOptions` carries `--tick-delay-ms`.
- `snapshotOf()` produces an immutable `TableSnapshot`, `TableScene` turns it into drawing calls, and `TableRenderSink` runs that once per `engine.tick`.
- `WindowCloseSource` and `WindowSetup` handle the window.

## Non-obvious decisions

**Only the money is recorded.**
The shuffle is seeded from `RoomConfig` and `PolicyAgent` is a pure function of the `TableView` it is handed, so cards and decisions are regenerated rather than stored — a replay file for a long session holds only the deposits, buy-ins and cash-outs.

**A buy-in cannot exceed a bankroll, structurally.**
`CashGame` is the only route between the two, so there is no second path where the check could be forgotten.

**Rendering is write-only in structure, not by promise.**
`TableSnapshot` is the spectator's answer to `holdem::TableView`: an immutable value handed to a scene that can only draw.
`TableRenderSink` is registered *after* `PokerRoomSink`, since that is what steps the table.

**Closing the window is input.**
`WindowCloseSource` is an `ITickSource` decorator that appends `engine.stop` once the window has gone, so a close lands in a `--record` file like anything else.

**The hand history derives from `StepOutcome`.**
`TablePrinter` reads the blinds, the raise sizes and the uncalled bet off what the table reported, rather than recomputing the betting — otherwise it would be a second implementation of the rules, free to drift.

See [`blog/010-a-poker-hand-in-one-number.md`](../../blog/010-a-poker-hand-in-one-number.md) and [`blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md`](../../blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md).
