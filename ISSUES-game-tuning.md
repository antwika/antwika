# game-tuning

## There is no walker spawner to double the rate of
**Task:** "Walkers spawn more often -- roughly double the rate (halve the interval)."
**Blocker:** `apps/game` has no automatic walker spawner and no spawn interval.
A walker exists only because somebody right-clicked a path tile: `game::GridSink::placeWalker()` is the one and only place a `Walker` component is ever added, and it runs off an `input.pointer_button_pressed` event.
Nothing in the app is periodic in that sense -- there is no timer, no rate, no budget and no interval constant anywhere near walkers.
The only per-tick interval in the app is `kTickInterval` in `main.cpp`, which is `TickPacer`'s wall-clock pacing for the whole run, not a spawn rate; halving it would double the speed of everything, including the walking cadence the other half of this task just halved.
**Question for the human:** What did you want doubled?
Three readings, all plausible, all different features:
(a) add a new automatic spawner that drops a walker every N ticks onto some path tile, in which case say which tile and what N;
(b) make one right-click drop two walkers;
(c) halve `kTickInterval` so the whole simulation runs twice as fast.
I did not pick one, because (a) is a new game mechanic rather than a tuning change, and inventing it would be inventing a requirement.
**What I did instead:** Nothing for this half.
The walking cadence change (one cell every two ticks) shipped on its own.
