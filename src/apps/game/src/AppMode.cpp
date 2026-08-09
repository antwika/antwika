#include "antwika/game/AppMode.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::game
{

    void AppModeState::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            current = staged;
        }
    }

}
