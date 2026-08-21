#include "antwika/time/ThreadSleeper.hpp"

#include <chrono>
#include <thread>

namespace antwika::time
{

    void ThreadSleeper::sleep(std::chrono::milliseconds duration)
    {
        std::this_thread::sleep_for(duration);
    }

}
