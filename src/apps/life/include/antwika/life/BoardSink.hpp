#pragma once

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>

#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class BoardSink final : public ITickEventSink
    {
    public:
        BoardSink(World &world, const Grid &grid, SystemScheduler &scheduler);

        BoardSink(const BoardSink &) = delete;
        BoardSink(BoardSink &&) = delete;

        BoardSink &operator=(const BoardSink &) = delete;
        BoardSink &operator=(BoardSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        World &world;
        const Grid &grid;
        SystemScheduler &scheduler;
    };

}
