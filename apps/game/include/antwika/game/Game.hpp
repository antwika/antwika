#pragma once

#include <antwika/engine/Engine.hpp>
#include <antwika/event/IEventDispatcher.hpp>

using antwika::engine::Engine;
using antwika::event::IEventDispatcher;

namespace antwika::game
{

    class Game
    {
    public:
        explicit Game(Engine &engine, IEventDispatcher &eventDispatcher);
        void run();

    private:
        Engine &engine;
        IEventDispatcher &eventDispatcher;
    };

} // namespace antwika::game
