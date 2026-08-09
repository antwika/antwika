#pragma once

#include <chrono>

#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::log::ILogger;
    using antwika::time::ISleeper;

    class PacingSink final : public ITickEventSink
    {
    public:
        PacingSink(
            ILogger &logger,
            ISleeper &sleeper,
            std::chrono::milliseconds interval);

        PacingSink(const PacingSink &) = delete;
        PacingSink(PacingSink &&) = delete;

        PacingSink &operator=(const PacingSink &) = delete;
        PacingSink &operator=(PacingSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        antwika::ecs::World world;
        antwika::simulation::TickPacer pacer;
    };

}
