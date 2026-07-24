#pragma once

#include <chrono>

namespace antwika::time
{

    class IClock
    {
    public:
        virtual ~IClock() = default;
        virtual std::chrono::time_point<std::chrono::system_clock> now() const = 0;
    };
} // namespace antwika::time
