#include "antwika/time/SystemSleeper.hpp"

#include <chrono>
#include <thread>

namespace antwika::time
{

    void SystemSleeper::sleep(std::chrono::milliseconds duration)
    {
        std::this_thread::sleep_for(duration);
    }

}
