#pragma once

#include <cstddef>
#include <vector>

namespace antwika::wfc
{

    /**
     * @brief The three possible outcomes of Solver::solve().
     */
    enum class SolveOutcome
    {
        Solved,
        Unsatisfiable,
        LimitExceeded,
    };

    /**
     * @brief The result of one Solver::solve() call.
     */
    struct SolveResult
    {
        SolveOutcome outcome;
        std::vector<std::size_t> assignment; // valid iff Solved
    };

} // namespace antwika::wfc
