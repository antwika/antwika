#pragma once

#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs::fakes
{

    template <typename PositionT>
    class FakeRecordPositionSystem final : public ISystem
    {
    public:
        FakeRecordPositionSystem(Entity entity, std::vector<int> &observedOrder)
            : entity(entity), observedOrder(observedOrder)
        {
        }

        void update(World &world, antwika::time::Tick) override
        {
            observedOrder.push_back(world.get<PositionT>(entity).x);
        }

    private:
        Entity entity;
        std::vector<int> &observedOrder;
    };

}
