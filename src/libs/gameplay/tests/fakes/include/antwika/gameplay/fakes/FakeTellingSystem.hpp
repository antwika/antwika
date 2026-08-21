#pragma once

#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/gameplay/fakes/UpdateCount.hpp"

namespace antwika::gameplay::fakes
{

    class FakeTellingSystem final : public ecs::ISystem
    {
    public:
        FakeTellingSystem(
            ecs::Entity entity, std::vector<int> &seenOrder)
            : entity(entity), seenOrder(&seenOrder)
        {
        }

        void update(ecs::World &world, time::Tick) override
        {
            seenOrder->push_back(world.get<UpdateCount>(entity).seenCount);
        }

    private:
        ecs::Entity entity;
        std::vector<int> *seenOrder;
    };

}
