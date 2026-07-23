#include "antwika/time/SystemClock.hpp"

namespace antwika::time
{
    std::chrono::time_point<std::chrono::system_clock> SystemClock::now() const
    {
        return std::chrono::system_clock::now();
    }
} // namespace antwika::time
