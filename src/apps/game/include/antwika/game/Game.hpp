#pragma once

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventQueue.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/log/IAppender.hpp>
#include <antwika/log/IFormatter.hpp>
#include <antwika/log/ILogPolicy.hpp>
#include <antwika/time/IClock.hpp>

namespace antwika::game
{

    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventQueue;
    using antwika::event::IEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogPolicy;
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

    void bootstrap(IClock &clock,
                    IAppender &appender,
                    IFormatter &formatter,
                    ILogPolicy &logPolicy,
                    IEventQueue &eventQueue,
                    IEventSink &eventSink);

} // namespace antwika::game
