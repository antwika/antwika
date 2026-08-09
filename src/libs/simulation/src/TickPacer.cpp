#include "antwika/simulation/TickPacer.hpp"

#include <chrono>

namespace antwika::simulation
{

    TickPacer::TickPacer(ISleeper &sleeper, std::chrono::milliseconds interval)
        : sleeper(sleeper), interval(interval)
    {
    }

    void TickPacer::update(World &, antwika::time::Tick)
    {
        sleeper.sleep(interval);
    }

}
