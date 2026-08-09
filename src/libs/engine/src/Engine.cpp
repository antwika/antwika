#include "antwika/engine/Engine.hpp"

#include <antwika/event/Event.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/engine/Events.hpp"

using antwika::event::Event;
using antwika::log::Level;

namespace antwika::engine
{

    Engine::Engine(ILogger &logger, IEventDispatcher &dispatcher)
        : logger(logger), dispatcher(dispatcher)
    {
    }

    void Engine::start()
    {
        logger.log(Level::Info, "Antwika engine started!");
    }

    void Engine::step(antwika::time::Tick)
    {
        dispatcher.dispatch(Event{.name = events::kTick}); // GCOVR_EXCL_LINE
    }

}
