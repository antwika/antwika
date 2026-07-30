#include "antwika/life/TickPacer.hpp"

#include <thread>

namespace antwika::life
{

    TickPacer::TickPacer(std::chrono::milliseconds interval)
        : interval(interval)
    {
    }

    void TickPacer::update(World &, antwika::time::Tick)
    {
        std::this_thread::sleep_for(interval);
    }

} // namespace antwika::life
