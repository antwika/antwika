#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs::fakes
{

    template <typename PositionT>
    class FakeSpawnSystem final : public ISystem
    {
    public:
        FakeSpawnSystem(Entity &spawnedEntity, int x)
            : spawnedEntity(&spawnedEntity), x(x)
        {
        }

        void update(World &world, antwika::time::Tick) override
        {
            *spawnedEntity = world.create();

            world.add<PositionT>(*spawnedEntity, PositionT{x});
        }

    private:
        Entity *spawnedEntity;
        int x;
    };

}
