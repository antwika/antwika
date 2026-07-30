#pragma once

#include <stdexcept>

namespace antwika::wfc
{

    /**
     * @brief Thrown by Solver's constructor for mismatched Domain
     * alphabet sizes across the wave, an out-of-range constraint cell
     * index, or an invalid valueWeights vector (wrong size, or a
     * non-positive weight).
     *
     * Also thrown by Domain::singleValue() on a domain that is not a
     * singleton.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::ecs::EcsError and antwika::replay::ReplayFormatError.
     * An unsatisfiable puzzle and a budget-exceeded search are both
     * normal, non-exceptional SolveResults, not errors, so the only
     * way solve() throws is a wave it was about to report Solved
     * turning out to hold an undetermined cell.
     */
    class WfcError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::wfc
