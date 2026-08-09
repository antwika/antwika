#include "antwika/game/ModeGatedSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::game
{

    ModeGatedSink::ModeGatedSink(
        ITickEventSink &inner,
        const AppModeState &mode,
        AppMode active) noexcept
        : inner(inner), mode(mode), active(active)
    {
    }

    void ModeGatedSink::handle(const TickEvent &event)
    {
        const bool isTick =
            event.event.name == antwika::engine::events::kTick;

        if (isTick || mode.mode() == active)
        {
            inner.handle(event);
        }
    }

}
