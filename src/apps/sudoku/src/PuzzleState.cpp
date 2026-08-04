#include "antwika/sudoku/PuzzleState.hpp"

#include <antwika/wfc/SolveResult.hpp>

#include "antwika/sudoku/Puzzle.hpp"

namespace antwika::sudoku
{

    using antwika::wfc::SolveOutcome;

    void PuzzleState::start(const Board &puzzle)
    {
        givens = puzzle;
        cells = puzzle;
        chosen.reset();
        note = Status::Playing;
    }

    const Board &PuzzleState::board() const noexcept
    {
        return cells;
    }

    const Board &PuzzleState::clues() const noexcept
    {
        return givens;
    }

    void PuzzleState::restore(
        const Board &puzzle,
        const Board &progress,
        const std::optional<Square> &pick,
        const Status said)
    {
        givens = puzzle;
        cells = progress;
        chosen = pick;
        note = said;
    }

    bool PuzzleState::isGiven(const Square square) const
    {
        return givens.at(square.row, square.col).has_value();
    }

    const std::optional<Square> &PuzzleState::selected() const noexcept
    {
        return chosen;
    }

    void PuzzleState::select(const Square square)
    {
        // Asked before the square is kept.
        // So an index outside the grid leaves the selection alone.
        const bool locked = isGiven(square);

        chosen = square;
        note = locked ? Status::GivenLocked : Status::Playing;
    }

    void PuzzleState::enter(const int digit)
    {
        if (!chosen.has_value())
        {
            return;
        }

        write(*chosen, digit);
    }

    void PuzzleState::write(const Square square, const int digit)
    {
        if (isGiven(square))
        {
            note = Status::GivenLocked;
            return;
        }

        cells.set(square.row, square.col, digit);
        noteCompletion();
    }

    void PuzzleState::solve(const antwika::wfc::SolverLimits limits)
    {
        const auto attempt = solvePuzzle(cells, limits);

        if (attempt.outcome == SolveOutcome::Unsatisfiable)
        {
            note = Status::Unsolvable;
            return;
        }

        if (attempt.outcome == SolveOutcome::LimitExceeded)
        {
            note = Status::LimitExceeded;
            return;
        }

        // The clues stay the clues.
        // A solved grid agrees with them everywhere.
        // So nothing about which squares were given changes.
        cells = attempt.board;
        note = Status::Solved;
    }

    Status PuzzleState::status() const noexcept
    {
        return note;
    }

    std::uint32_t PuzzleState::filled() const
    {
        std::uint32_t count = 0;

        for (std::size_t row = 0; row < Board::kSize; ++row)
        {
            for (std::size_t col = 0; col < Board::kSize; ++col)
            {
                if (cells.at(row, col).has_value())
                {
                    ++count;
                }
            }
        }

        return count;
    }

    void PuzzleState::noteCompletion()
    {
        note = isComplete(cells) ? Status::Complete : Status::Playing;
    }

} // namespace antwika::sudoku
