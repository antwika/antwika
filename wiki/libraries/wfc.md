# antwika::wfc

`src/libs/wfc/` — Wave Function Collapse as constraint propagation.

## What it is for

Solving a constraint problem over a flat array of cells, each of which starts with a set of possible values and is narrowed until one remains.
It is standalone and dependency-free — it links no other `antwika` library and knows nothing about ticks, events or replay.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Solver.hpp` | `Solver` | Constructed with the cells, the constraints and the limits; `solve()` returns a `SolveResult`. |
| `Domain.hpp` | `Domain` | The set of values a cell may still take, with a `const_iterator` over them. |
| `IConstraint.hpp` | `IConstraint` | The only way geometry enters the library. |
| `AllDifferentConstraint.hpp` | `AllDifferentConstraint` | No two cells in the named group share a value. |
| `AdjacencyConstraint.hpp` | `AdjacencyConstraint` | Two cells' values must be compatible, per a `CompatibilityTable`. |
| `CompatibilityTable.hpp` | `CompatibilityTable` | Which value pairs may sit next to each other. |
| `SolveResult.hpp` | `SolveResult`, `SolveOutcome` | The solved cells and how the solve ended. |
| `SolverLimits.hpp` | `SolverLimits` | Bounds on the work a solve may do. |
| `WfcError.hpp` | `WfcError` | Malformed input, such as a constraint naming a cell that does not exist. |

`MockConstraint` lives under `tests/mocks/`.

## Depends on

Nothing.

## Non-obvious decisions

**There is no grid inside the library.**
Cells are a flat, index-addressed `std::vector`, and *all* geometry — rows, columns, boxes, neighbours — is expressed by the constraints a caller supplies.
That is why the same solver serves a Sudoku (81 cells and 27 `AllDifferentConstraint`s, in [`apps/sudoku`](../apps/sudoku.md)) and a tiling problem with no changes.

**The solver is deterministic, and reports rather than guesses.**
`SolveOutcome` distinguishes a solved problem, a contradiction and a solve that ran out of budget, so a caller learns what actually happened instead of being handed a plausible-looking answer.
An earlier version that quietly guessed is the subject of [`blog/008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md`](../../blog/008-the-solver-that-lied-the-zombie-component-and-a-coverage-gate-with-teeth.md).

See also [`blog/005-wave-function-collapse-that-never-guesses.md`](../../blog/005-wave-function-collapse-that-never-guesses.md).
