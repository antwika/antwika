#pragma once

#include <cstdint>
#include <optional>

namespace antwika::wfc
{

    struct SolverLimits final
    {
        std::optional<std::uint64_t> maxSteps;
    };

}
