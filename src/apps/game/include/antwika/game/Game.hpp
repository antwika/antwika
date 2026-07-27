#pragma once

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventQueue.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/log/IAppender.hpp>
#include <antwika/log/IFormatter.hpp>
#include <antwika/log/ILogPolicy.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GameState.hpp"

namespace antwika::game
{

    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventQueue;
    using antwika::event::IEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogPolicy;
    using antwika::replay::IReplaySource;
    using antwika::time::IClock;

    class Game
    {
    public:
        explicit Game(IEngine &engine, IEventDispatcher &dispatcher);

        Game(const Game &) = delete;
        Game(Game &&) = delete;

        Game &operator=(const Game &) = delete;
        Game &operator=(Game &&) = delete;

        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    // Wires the engine/event/replay collaborators together, boots the game,
    // then drives the fixed-timestep tick loop for totalTicks, sourcing that
    // tick's events from inputSource -- a hand-scripted "live" run and a
    // loaded replay both go through this identical function, differing only
    // in what inputSource was built from. Returns the resulting GameState so
    // callers (main.cpp, tests) can inspect or compare it.
    GameState bootstrap(IClock &clock,
                        IAppender &appender,
                        IFormatter &formatter,
                        ILogPolicy &logPolicy,
                        IEventQueue &eventQueue,
                        IEventSink &eventSink,
                        IReplaySource &inputSource,
                        antwika::time::Tick totalTicks);

} // namespace antwika::game
