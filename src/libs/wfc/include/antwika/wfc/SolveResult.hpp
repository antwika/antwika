#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace antwika::wfc
{

    enum class SolveOutcome : std::uint8_t
    {
        Solved,
        Unsatisfiable,
        LimitExceeded,
    };

    struct SolveResult final
    {
        SolveOutcome outcome;
        std::vector<std::size_t> assignment;
    };

}
