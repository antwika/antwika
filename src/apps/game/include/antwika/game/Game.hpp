#pragma once

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>

namespace antwika::game
{

    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;

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
