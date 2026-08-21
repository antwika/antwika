#pragma once

#include <optional>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/engine/IEngine.hpp"
#include "antwika/engine/StopSignal.hpp"

namespace antwika::engine
{

    using antwika::event::ITickEventSource;
    using antwika::event::TickedEventDispatcher;

    class EngineLoop final
    {
    public:
        EngineLoop(
            IEngine &engine,
            TickedEventDispatcher &dispatcher,
            ITickEventSource &source);

        EngineLoop(const EngineLoop &) = delete;
        EngineLoop(EngineLoop &&) = delete;

        EngineLoop &operator=(const EngineLoop &) = delete;
        EngineLoop &operator=(EngineLoop &&) = delete;

        void run(
            const StopSignal &stop,
            std::optional<antwika::time::Tick> maxTicks = std::nullopt);

    private:
        IEngine &engine;
        TickedEventDispatcher &dispatcher;
        ITickEventSource &source;
    };

}
