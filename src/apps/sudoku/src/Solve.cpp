#include "antwika/sudoku/Solve.hpp"

#include <cstddef>
#include <functional>
#include <vector>

#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/Solver.hpp>

#include "antwika/sudoku/Puzzle.hpp"

namespace antwika::sudoku
{

    using antwika::wfc::IConstraint;
    using antwika::wfc::SolveOutcome;
    using antwika::wfc::Solver;

    SolveAttempt solvePuzzle(
        const Board &board, const antwika::wfc::SolverLimits limits)
    {
        const auto constraints = buildConstraints();
        std::vector<std::reference_wrapper<const IConstraint>> refs(
            constraints.begin(), constraints.end());

        const Solver solver(
            buildInitialWave(board), refs, {}, limits);
        const auto result = solver.solve();

        if (result.outcome != SolveOutcome::Solved)
        {
            return SolveAttempt{.outcome = result.outcome, .board = {}};
        }

        Board solved;
        for (std::size_t row = 0; row < Board::kSize; ++row)
        {
            for (std::size_t col = 0; col < Board::kSize; ++col)
            {
                const std::size_t index = row * Board::kSize + col;

                // The alphabet is 0-8 and the digits are 1-9.
                solved.set(
                    row,
                    col,
                    static_cast<int>(result.assignment[index]) + 1);
            }
        }

        return SolveAttempt{
            .outcome = SolveOutcome::Solved, .board = solved};
    } // GCOVR_EXCL_LINE

} // namespace antwika::sudoku
