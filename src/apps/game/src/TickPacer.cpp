#include "antwika/game/TickPacer.hpp"

#include <chrono>

namespace antwika::game
{

    TickPacer::TickPacer(ISleeper &sleeper, std::chrono::milliseconds interval)
        : sleeper(sleeper), interval(interval)
    {
    }

    void TickPacer::update(World &, antwika::time::Tick)
    {
        sleeper.sleep(interval);
    }

} // namespace antwika::game
