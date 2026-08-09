#pragma once

#include <chrono>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::simulation
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::time::ISleeper;

    class TickPacer final : public ISystem
    {
    public:
        TickPacer(ISleeper &sleeper, std::chrono::milliseconds interval);

        TickPacer(const TickPacer &) = delete;
        TickPacer(TickPacer &&) = delete;

        TickPacer &operator=(const TickPacer &) = delete;
        TickPacer &operator=(TickPacer &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        ISleeper &sleeper;
        std::chrono::milliseconds interval;
    };

}
