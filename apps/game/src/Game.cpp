#include "antwika/game/Game.hpp"

#include <iostream>
#include <antwika/time/SystemClock.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/Event.hpp>

namespace antwika::game
{

    Game::Game(Engine &engine,
               IEventDispatcher &dispatcher) : engine(engine),
                                               eventDispatcher(eventDispatcher)
    {
    }

    void Game::run()
    {
        engine.start();
    }

} // namespace antwika::game
