#include "antwika/game/CityEntrySink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::game
{

    CityEntrySink::CityEntrySink(
        const AppModeState &mode, PauseState &pause) noexcept
        : mode(mode), pause(pause), last(mode.mode())
    {
    }

    void CityEntrySink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        // The mode this tick ended in, since AppModeState runs first.
        const AppMode now = mode.mode();
        const AppMode was = last;
        last = now;

        if (now == AppMode::CityMap && was != AppMode::CityMap)
        {
            pause.hold();
        }
    }

} // namespace antwika::game
