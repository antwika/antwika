#pragma once

#include <chrono>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::system
{

    class TickPacer final : public ecs::ISystem
    {
    public:
        TickPacer(
            time::ISleeper &sleeper, std::chrono::milliseconds interval);

        TickPacer(const TickPacer &) = delete;
        TickPacer(TickPacer &&) = delete;

        TickPacer &operator=(const TickPacer &) = delete;
        TickPacer &operator=(TickPacer &&) = delete;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        time::ISleeper &sleeper;
        std::chrono::milliseconds interval;
    };

}
