#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/gameplay/fakes/UpdateCount.hpp"

namespace antwika::gameplay::fakes
{

    class FakeRaisingSystem final : public ecs::ISystem
    {
    public:
        explicit FakeRaisingSystem(ecs::Entity entity)
            : entity(entity)
        {
        }

        void update(ecs::World &world, time::Tick) override
        {
            world.set<UpdateCount>(
                entity,
                UpdateCount{world.get<UpdateCount>(entity).seenCount + 1});
        }

    private:
        ecs::Entity entity;
    };

}
