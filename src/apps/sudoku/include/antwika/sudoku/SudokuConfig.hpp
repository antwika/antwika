#pragma once

#include <cstdint>

#include "antwika/sudoku/Solve.hpp"

namespace antwika::sudoku
{

    struct SudokuConfig final
    {
        std::uint64_t solveStepBudget = kSolveStepBudget;
        std::int32_t framePeriodMs = 40;
    };

}
