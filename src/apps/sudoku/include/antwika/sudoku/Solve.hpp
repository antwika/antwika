#pragma once

#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/SolverLimits.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    inline constexpr std::uint64_t kSolveStepBudget = 200000;

    struct SolveAttempt final
    {
        antwika::wfc::SolveOutcome outcome =
            antwika::wfc::SolveOutcome::Unsatisfiable;

        Board board{};
    };

    [[nodiscard]] SolveAttempt solvePuzzle(
        const Board &board,
        antwika::wfc::SolverLimits limits = {
            .maxSteps = kSolveStepBudget});

}
