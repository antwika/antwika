# apps/sudoku

`src/apps/sudoku/` — a Sudoku you play with a mouse and a keyboard, finished by a solver that never guesses.

## What it demonstrates

[`wfc`](../libraries/wfc.md) put to work inside a tick loop.
The solver is still the point: the 81 squares are a flat wave, the rules are 27 `AllDifferentConstraint`s, and the library has no idea it is solving a Sudoku.
What is around it is an ordinary application of the tick loop — a window, recorded input, an immediate-mode UI, and a replay that reproduces a session by construction.

## Running it

```sh
build/bin/antwika_sudoku/antwika_sudoku
build/bin/antwika_sudoku/antwika_sudoku --puzzle my-puzzle.txt
build/bin/antwika_sudoku/antwika_sudoku --max-ticks 0
build/bin/antwika_sudoku/antwika_sudoku --record demo.replay
build/bin/antwika_sudoku/antwika_sudoku --replay src/apps/sudoku/replays/demo.json
```

Click a square to pick it, type `1`-`9` to write a digit, and press backspace or delete to empty it again.
A clue cannot be overwritten and says so rather than doing nothing.
**Solve** finishes the rest of the grid from wherever the play has got to; a grid somebody has made contradictory comes back "no solution exists from here" and is left exactly as it was.

Without `--puzzle` it plays a well-known easy puzzle baked into `PuzzleFile.hpp`.
A missing or unreadable file is reported rather than read as an empty puzzle, and a malformed one raises `BoardFormatError`.

**A puzzle has no end of its own**, so a session runs until the window is closed, Escape is pressed, a replay dispatches `engine.stop`, or `--max-ticks` runs out — 90000 ticks by default, which is an hour at the 40 ms frame period.
The default `null` backend reports neither a close nor a key, and every CI leg builds that one, so the cap is what makes a headless run finish at all; `--max-ticks 0` removes it, for somebody in front of a real window with a hard puzzle.
Since a `--record` run only writes its file once the run has ended, a run killed with `Ctrl+C` saves nothing.

## Libraries it composes

[`app`](../libraries/app.md), [`cli`](../libraries/cli.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`gfx`](../libraries/gfx.md), [`i18n`](../libraries/i18n.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`simulation`](../libraries/simulation.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md), [`wfc`](../libraries/wfc.md), plus the selected backends.

## How it is put together

`Board` parses and prints the 81 squares; `Puzzle` expresses them for the solver and answers whether a grid still obeys the rules; `solvePuzzle()` in `Solve.hpp` is the console showcase this application used to be, kept as one function.
`PuzzleState` is everything a session holds: the clues, the working grid, the picked square and the last thing it said.
`BoardSink` folds this application's own `sudoku.*` events; `PlaySink` folds `antwika::input`'s and describes the picture; `SudokuScene` is the picture, `BoardOverlay` carries it to `RenderSink`.
`PuzzleSource` and `TickLimitSource` are the two decorators on the event stream, and `bootstrap()` in `Sudoku.cpp` wires the lot.

## Non-obvious decisions

**The solver has no idea it is solving a Sudoku.**
`antwika::wfc` holds a flat `std::vector` of cells and knows nothing about rows, columns or boxes; *all* the geometry is in the constraints this app supplies.
That is what makes the same solver reusable for a tiling problem with no changes to the library.

**It never guesses.**
`SolveOutcome` distinguishes a solved grid, a contradiction and an exhausted budget, so "no solution exists from here" is a real answer rather than a plausible-looking board.
That is also what makes the Solve button honest about a grid the player has broken: the contradiction is reported and the grid is left alone.

**A solve is bounded, because it runs inside a tick.**
`kSolveStepBudget` caps the candidate values one press may try, in *steps* rather than milliseconds.
A wall-clock bound would make the answer depend on the machine, and a replay would then diverge from the run it replays — the same reason `antwika::wfc` counts steps in the first place.
An unbounded solve on an adversarially filled grid would not merely take a while; it would stop the loop, and a tick that never returns draws no frames and reads no input.

**Nothing here draws a random bit, and that is the strongest form of the determinism rule.**
`antwika::wfc` breaks every tie by ascending cell index and takes no seed at all, so the same grid gives the same solution on every toolchain.
There is therefore no `antwika::rng` in this application and nothing for a recording to have to carry: "randomness only ever comes from an injected rng seeded from something already persisted" is met by there being no randomness.

**The puzzle travels as an event, not as a constructor argument.**
A puzzle arrives from outside the program — a `--puzzle` file, or the demo constant — so it is external input, and external input reaches a simulation through the source the loop pulls from.
`PuzzleSource` puts one `sudoku.new_puzzle` at the head of the first tick, *upstream of the recorder*, so a `--record` file carries the grid it was played on and replaying it needs no flag typed again.
A replay announces nothing: the recording already has its puzzle, and a second one would change the grid halfway through the session the clicks were aimed at.

**Three events, and none of them holds anything this application can work out again.**
`sudoku.new_puzzle` carries a grid nobody could regenerate, `sudoku.set_cell` carries what a script asked for, and `sudoku.solve` records that a solve was *asked for* — never the solution it produced.
The digit a keystroke put in a square, the squares a solve filled and whether the grid is finished are all regenerated, which is why there is no `sudoku.cell_filled` and no `sudoku.solved`.
`ReplayIntegrationTest` records a click, a keystroke and a press of Solve, writes them through the real `saveReplayFile()`/`loadReplayFile()` pair, replays them and asserts the same 81 characters come out.

**One hit-test decides what a press means.**
The Solve button and the board's area are both named widgets of the same `ui` frame, so `antwika::ui` reports which of them the press was topmost over, and only a press the board answered for is mapped to a square.
A second, independent test of "was this over the bar" is exactly the drift `ui::Frame::rects` exists to prevent.
The grid itself is *appended* to that frame from the rectangle the board container reported, rather than laid out beside it, so the bar's height and the grid's position cannot disagree.

**Where a square is drawn and which square a click lands in are one function.**
`layoutFor()`, `cellAt()` and `squareRect()` in `BoardLayout.hpp` are shared by `SudokuScene` and `PlaySink`, following `life::BoardLayout` and for the same reason.
That mapping is against the *configured* window size, and the window is not resizable, which is what keeps a recorded session landing on the same squares under a different backend.

**The clues are kept beside the working grid rather than inferred from it.**
"This square was given" stops being visible the moment somebody types the same digit into an empty one, and a clue that could be overwritten is a puzzle that can be edited into an easier one.

**A puzzle file states its version, and the classic flat grid is version 1 of it.**
`PuzzleFile.cpp` reads a document `parse -> read version -> migrate -> validate -> decode`, through an injected `replay::MigrationChain` like every other persisted format here.
Version 1 is the 81-character grid the rest of the world writes, which predates the mechanism and says nothing about a version — precisely what `kUnversionedDocumentVersion` is for — so no file on anybody's disk had to change.
Version 2 wraps it in an object carrying `"magic": "antwika-sudoku"`, because a replay and a companion save state their version in the same member and the magic is the only thing telling the three apart.

Which of the two shapes a document is, is decided on its first non-space character rather than by trying JSON and falling back: a finished grid is 81 digits, and 81 digits *are* valid JSON — a number — so the sniff would misread the one puzzle that needs no solving.

**`--locale` is gone, and that is the point.**
This used to be the one application allowed to take a locale at run time, because it neither recorded input nor hit-tested a layout.
It now does both: the Solve button is as wide as its own label and the grid sits under whatever height the bar comes to, so a session recorded in one language and replayed in another would resolve the same click to a different square.
The locale is fixed at `i18n::kDefaultLocale` in `main()` and read from nowhere else, exactly as every other windowed application here does it.

**A status is an id, not a sentence.**
`PuzzleState` holds a `Status`, and `SudokuScene` is the only place in this application that has heard of a language — following `atlas_editor::StatusMessage` and `ui_demo::DemoMessage`.
Storing the words would put the active locale inside the state a replay reproduces.

See [`blog/005-wave-function-collapse-that-never-guesses.md`](../../blog/005-wave-function-collapse-that-never-guesses.md) and [`blog/008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md`](../../blog/008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md), which describe this application while it was still a console showcase.
