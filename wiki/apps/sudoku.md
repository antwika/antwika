# apps/sudoku

`src/apps/sudoku/` — a Sudoku solved as constraint propagation.

## What it demonstrates

[`wfc`](../libraries/wfc.md) on its own.
This app is unrelated to the tick and replay system: no engine, no events, no window, no backend.
It reads a puzzle, builds constraints, solves, and prints.

## Running it

```sh
build/bin/antwika_sudoku/antwika_sudoku
build/bin/antwika_sudoku/antwika_sudoku --puzzle my-puzzle.txt
```

Without `--puzzle` it solves a well-known easy demo puzzle baked into `main.cpp`.
A missing or unreadable file is reported rather than read as an empty puzzle, and a malformed one raises `BoardFormatError`.
If the constraints admit no solution, it says so instead of printing a guess.

## Libraries it composes

[`wfc`](../libraries/wfc.md), and nothing else.

## How it is put together

`Board` parses and prints the 81-cell grid; `Puzzle` expresses it for the solver.
The rules are 27 `AllDifferentConstraint`s over a flat, index-addressed array — nine rows, nine columns and nine boxes.

## Non-obvious decisions

**The solver has no idea it is solving a Sudoku.**
`antwika::wfc` holds a flat `std::vector` of cells and knows nothing about rows, columns or boxes; *all* the geometry is in the constraints this app supplies.
That is what makes the same solver reusable for a tiling problem with no changes to the library.

**It never guesses.**
The solver reports a contradiction or an exhausted budget through `SolveOutcome` rather than producing a plausible-looking board, which is why "no solution exists for this puzzle" is a real answer here.

See [`blog/005-wave-function-collapse-that-never-guesses.md`](../../blog/005-wave-function-collapse-that-never-guesses.md) and [`blog/008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md`](../../blog/008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md).
