#pragma once

#include <chrono>

namespace antwika::time
{

    class IClock
    {
    public:
        virtual ~IClock() = default;

        [[nodiscard]] virtual std::chrono::time_point<std::chrono::system_clock>
        getCurrentTime() const noexcept = 0;
    };
}
