#pragma once

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>

using antwika::engine::IEngine;
using antwika::event::IEventDispatcher;

namespace antwika::game
{

    class Game
    {
    public:
        explicit Game(IEngine &engine, IEventDispatcher &eventDispatcher);
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &eventDispatcher;
    };

} // namespace antwika::game
