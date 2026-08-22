#pragma once

#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs::fakes
{

    class FakeRecordAliveSystem final : public ISystem
    {
    public:
        FakeRecordAliveSystem(
            const Entity &watchedEntity, std::vector<bool> &seenAlive)
            : watchedEntity(&watchedEntity), seenAlive(&seenAlive)
        {
        }

        void update(World &world, antwika::time::Tick) override
        {
            seenAlive->push_back(world.alive(*watchedEntity));
        }

    private:
        const Entity *watchedEntity;
        std::vector<bool> *seenAlive;
    };

}
