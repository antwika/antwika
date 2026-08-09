#pragma once
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::engine::events
{

    using antwika::event::ITickEventSource;

    inline constexpr const char *kTick = "engine.tick";

    inline constexpr const char *kStop = "engine.stop";

}
