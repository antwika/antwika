#include "antwika/replay/EngineLoop.hpp"

#include <utility>

namespace antwika::replay
{

    EngineLoop::EngineLoop(IEngine &engine, TickedEventDispatcher &dispatcher, IReplaySource &source) : engine(engine), dispatcher(dispatcher), source(source)
    {
    }

    void EngineLoop::run(antwika::time::Tick totalTicks)
    {
        for (antwika::time::Tick tick = 0; tick < totalTicks; ++tick)
        {
            dispatcher.setTick(tick);

            for (auto &event : source.eventsFor(tick))
            {
                dispatcher.dispatch(std::move(event));
            }

            engine.step(tick);
        }
    }

} // namespace antwika::replay
