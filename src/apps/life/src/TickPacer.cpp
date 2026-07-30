#include "antwika/life/TickPacer.hpp"

#include <chrono>

namespace antwika::life
{

    TickPacer::TickPacer(ISleeper &sleeper, std::chrono::milliseconds interval)
        : sleeper(sleeper), interval(interval)
    {
    }

    void TickPacer::update(World &, antwika::time::Tick)
    {
        sleeper.sleep(interval);
    }

} // namespace antwika::life
