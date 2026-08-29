#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/system/SimulationState.hpp"

namespace antwika::system
{

    class PadSystem final : public ecs::ISystem
    {
    public:
        explicit PadSystem(const SimulationState &simulation) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const SimulationState *simulation;
    };

}
