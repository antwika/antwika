#pragma once

#include <cstddef>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/system/SimulationState.hpp"

namespace antwika::system
{

    inline constexpr float kTalkRadius = 1.6F;

    class TalkSystem final : public ecs::ISystem
    {
    public:
        explicit TalkSystem(const SimulationState &simulation) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const SimulationState *simulation;
    };

}
