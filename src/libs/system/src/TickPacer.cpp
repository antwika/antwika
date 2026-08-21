#include "antwika/system/TickPacer.hpp"

#include <chrono>

namespace antwika::system
{

    TickPacer::TickPacer(
        time::ISleeper &sleeper, const std::chrono::milliseconds interval)
        : sleeper(sleeper), interval(interval)
    {
    }

    void TickPacer::update(ecs::World &, time::Tick)
    {
        sleeper.sleep(interval);
    }

}
