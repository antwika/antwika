#pragma once

#include <cstdint>

namespace antwika::scheduler
{

    enum class JobId : std::uint64_t
    {
    };

    inline constexpr JobId kInvalidJobId{0};

    [[nodiscard]] constexpr std::uint64_t rawValue(JobId jobId) noexcept
    {
        return static_cast<std::uint64_t>(jobId);
    }

}
