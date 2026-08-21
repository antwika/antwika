#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs::fakes
{

    template <typename PositionT>
    class FakeSetPositionSystem final : public ISystem
    {
    public:
        FakeSetPositionSystem(Entity entity, int x) : entity(entity), x(x)
        {
        }

        void update(World &world, antwika::time::Tick) override
        {
            world.set<PositionT>(entity, PositionT{x});
        }

    private:
        Entity entity;
        int x;
    };

}
