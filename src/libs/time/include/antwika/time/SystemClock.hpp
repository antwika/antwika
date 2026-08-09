#pragma once

#include <chrono>

#include "IClock.hpp"

namespace antwika::time
{

    class SystemClock final : public IClock
    {
    public:
        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock>
        now() const noexcept override;
    };
}
