#pragma once

#include <cstdint>
#include <optional>

namespace antwika::wfc
{

    /**
     * @brief A deterministic, step-counted bound on search work.
     *
     * One "step" is one candidate value attempted at a decision point --
     * never wall-clock time, matching the project-wide "discrete steps,
     * not wall-clock time" principle.
     */
    struct SolverLimits
    {
        std::optional<std::uint64_t> maxSteps;
    };

} // namespace antwika::wfc
