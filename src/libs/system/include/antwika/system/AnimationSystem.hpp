#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::system
{

    class AnimationSystem final : public ecs::ISystem
    {
    public:
        void update(ecs::World &world, time::Tick tick) override;
    };

}
