# antwika::sequencer

`src/libs/sequencer/` — where musical time meets frame time.

## What it is for

Turning a [`pattern`](pattern.md) into things that begin at exact frames, once per tick.

It is the only place the two clocks meet, and it is deliberately small.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Rational.hpp` | `Rational` | The exact fraction, which is `pattern::Cycle` under a second name. |
| `FrameClock.hpp` | `FrameClock` | A tick, to the frame it begins on. |
| `TempoMap.hpp` | `TempoMap` | A cycle to a frame and back, across tempo changes; `segments()` reads the table back, so a dump can rebuild an equal map by replaying `addSegment()`. |
| `ISequencerSink.hpp` | `ISequencerSink` | Where what begins is handed. |
| `Sequencer.hpp` | `Sequencer`, `SequencerDesc` | The window, the query, the onset rule, and `joinAt()` for a voice arriving late. |
| `SequencerError.hpp` | `SequencerError` | This library's one failure type. |

## Depends on

[`pattern`](pattern.md), [`sound`](sound.md) for `FrameIndex`, and [`time`](time.md) for `Tick`.

**Not [`synth`](synth.md)**, which is the point of `ISequencerSink`: this library says *what* begins and *when*, and deciding that a control means a frequency is the application's business.

## Non-obvious decisions

**`joinAt()` is how a voice arrives in the middle of a run.**
A sequencer built fresh has been asked about nothing, so its first `advance()` covers every cycle from zero to now and hands on every onset in all of them at once -- which is right for a run starting at tick zero and catastrophic for a voice that a caller has just made, five thousand ticks in.
`joinAt(tick)` declares everything through that tick's window already asked for, without querying the pattern and without sounding anything, and it cannot move the window backwards.
[music_editor](../apps/music_editor.md) is what wanted it: a voice there is a line of a document, so voices appear and disappear as somebody types, and the past is not a new one's to play.



**Three clocks, two exact maps, and no floating point in either.**
`time::Tick` is when decisions are made, `sound::FrameIndex` is what everything is placed against, and `Cycle` is what music is written in.
`FrameClock` maps the first to the second and `TempoMap` maps the third to the second, both by exact integer arithmetic.

**The residue lives in the expression, never in a running variable.**
Frames per tick is an exact fraction and a tick's frame is worked out from that tick alone, so nothing accumulates and nothing can lose count.
That is the whole answer to "the sample rate does not divide evenly into the tick rate": 44100 at a 3 ms tick alternates 132 and 133 forever and lands exactly on 1323 at tick ten, and on 132300000 at tick a million.

**The window is tracked in cycles, not frames.**
A frame maps back to a cycle through a division that need not land exactly, and a boundary that drifted by one frame would either sound an event twice or drop it.
Tracking the cursor in cycle space makes the window half-open and monotone by construction.

**It triggers on onsets, never on events.**
A pattern hands back a fragment for every event a window cut, and a held note produces one in every window it spans.
Sounding those restarts the note at every boundary, which is how a port of this idea comes out sounding like a machine gun.
`NeverSoundsAHeldNoteTwice` is the test worth the most in this library.

**A sink takes three arguments rather than one struct.**
A struct would have to own its controls, which is an allocation for every note on a path that runs as long as the program does.
The sink borrows them for the call and copies only if it wants to keep one.

**A position before the first tempo segment is extrapolated rather than refused.**
A pattern shifted early legitimately asks about cycles before zero, and the only thing below frame zero is frame zero.

**`retime()` is how tempo changes mid-run, and it cannot reach the past.**
It adds a `TempoMap` segment from a position at or after `queriedThrough()` and refuses anything earlier, because the notes a queried window handed on already carry the frames the old timeline gave them, and moving those frames would be an edit to something the mixer may already be sounding.
[music_editor](../apps/music_editor.md)'s speed box is the caller: one boundary per change, at the next whole cycle no voice has been asked past.

## What a replay sees of all this

**Nothing musical is ever recorded, and nothing needs to be.**
A pattern query is a pure function of a window, and a trigger is *derived* from simulation state, so both are regenerated exactly as a click's consequences are.
No `music.*` or `sfx.*` event name may exist, for the same reason no `ui.*` one does.

Musical position is derived from `Tick` through the two exact maps above, and never from `IDevice::framesPlayed()`, which remains legal only for deciding how long to sleep.

**A note's placement does not depend on the tick rate, and a game-triggered effect's does.**
A note's frame comes from the `TempoMap`, so it is sample-accurate however coarsely the window is queried, and the tick decides only *when* the query happens.
An effect the game fires is different in kind, because the decision to fire it is itself made on a tick: it can be placed no earlier than that tick's frame, so it carries up to one whole interval of latency and jitter -- 80 ms in [`tower_defence`](../apps/tower_defence.md), against a threshold nearer 10 ms.
**That is accepted rather than worked around**, and it is written here so that a hit sounding late is recognised as this rather than chased as a bug.
The only lever is the app's own tick interval, and raising one costs the retuning of every tick-denominated constant the app was tuned with -- and not the validity of its recordings, which store no tick rate and reproduce identical state whatever the pacing.

## See also

- [`pattern`](pattern.md) — what it queries.
- [`synth`](synth.md) — what an application usually points it at.
- [`notation`](notation.md) — where the pattern usually comes from.
