# game-palette

## The ghost cannot follow a freely moving pointer
**Task:** Task 2, the placement ghost.
**Blocker:** `main.cpp` attaches `input::IdleMotionSource`, which holds
back pointer movement while no button is held.
The ghost is therefore restated only when a click, a wheel or a key
arrives, so it jumps to where the last click was rather than gliding
under the cursor.
That is the trade CLAUDE.md already describes for the toolbar's hover
appearance, now visible on the grid as well.
I did not touch `main.cpp`, both because another agent owns it and
because removing the gate is a recording-size decision, not a rendering
one.
**Question for the human:** is a live ghost worth what taking the gate
out costs a `--record` file?
A middle answer exists: keep the gate but let it release a latched
movement once per tick even with no button held, which would cost one
recorded movement per tick instead of one per reported pixel.
**What I did instead:** shipped the ghost as it is, and said so in
`BuildGhost`'s and `GridSink`'s documentation so nobody reads the
behaviour as a bug.

## A building is not in the run summary
**Task:** Task 1, the build palette.
**Blocker:** `GameSummary` and `Game.cpp` are owned by other agents this
session, so `printSummary()` still reports only paths and walkers.
**Question for the human:** should a finished run print what was built,
the way it prints "Paths laid" and "Walkers"?
**What I did instead:** left both alone.
The buildings are in the `World` and in `SceneSnapshot`, so adding two
lines to `GameSummary` and `printSummary()` later needs no change here.
