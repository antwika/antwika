#pragma once

#include <chrono>

namespace antwika::time
{

    class IClock
    {
    public:
        virtual ~IClock() = default; // GCOVR_EXCL_LINE
        [[nodiscard]] virtual std::chrono::time_point<std::chrono::system_clock> now() const noexcept = 0;
    };
} // namespace antwika::time
