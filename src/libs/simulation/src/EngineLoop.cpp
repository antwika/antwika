#include "antwika/simulation/EngineLoop.hpp"
#include <antwika/event/ITickEventSource.hpp>

#include <utility>

#include "antwika/simulation/EngineLoopError.hpp"

namespace antwika::simulation
{

    using antwika::event::ITickEventSource;

    EngineLoop::EngineLoop(
        IEngine &engine,
        TickedEventDispatcher &dispatcher,
        ITickEventSource &source)
        : engine(engine), dispatcher(dispatcher), source(source)
    {
    }

    void EngineLoop::run(
        const StopSignal &stop, std::optional<antwika::time::Tick> maxTicks)
    {
        for (antwika::time::Tick tick = 0;; ++tick)
        {
            if (maxTicks.has_value() && tick >= *maxTicks)
            {
                throw EngineLoopError(
                    "EngineLoop::run reached maxTicks without an "
                    "engine.stop event");
            }

            dispatcher.setTick(tick);

            for (auto &event : source.eventsFor(tick))
            {
                dispatcher.dispatch(std::move(event));
            }

            engine.step(tick);

            if (stop.stopped())
            {
                break;
            }
        }
    }

} // namespace antwika::simulation
