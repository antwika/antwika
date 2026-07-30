#include "antwika/time/SystemSleeper.hpp"

#include <chrono>
#include <thread>

namespace antwika::time
{

    void SystemSleeper::sleep(std::chrono::milliseconds duration)
    {
        // No guard on a zero or negative duration.
        // sleep_for already returns immediately for both.
        std::this_thread::sleep_for(duration);
    }

} // namespace antwika::time
