#include "antwika/companion/PacingSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::companion
{

    PacingSink::PacingSink(
        ILogger &logger,
        ISleeper &sleeper,
        const std::chrono::milliseconds interval)
        : world(logger), pacer(sleeper, interval)
    {
    }

    void PacingSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        pacer.update(world, event.tick);
    }

} // namespace antwika::companion
