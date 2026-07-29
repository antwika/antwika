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
        // gcov -b tags this call's exception-unwind edge (throw).
        // It covers allocation inside Event's std::string members.
        // It also covers allocation inside dispatch() itself.
        // Taken only if one of those allocations actually fails.
        // See docs/confirming-unreachable-branches.md.
        dispatcher.dispatch(Event{.name = events::kTick}); // GCOVR_EXCL_LINE
    }

} // namespace antwika::engine
