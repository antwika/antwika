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

        Game(const Game &) = delete;
        Game(Game &&) = delete;

        Game &operator=(const Game &) = delete;
        Game &operator=(Game &&) = delete;

        void run();

    private:
        IEngine &engine;
        IEventDispatcher &eventDispatcher;
    };

} // namespace antwika::game
