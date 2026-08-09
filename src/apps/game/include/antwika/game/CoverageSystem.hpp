#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class CoverageSystem final : public ISystem
    {
    public:
        void update(World &world, antwika::time::Tick tick) override;
    };

}
