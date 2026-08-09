#include "antwika/game/Staff.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    std::int32_t staffCount(const Staff &staff)
    {
        std::int32_t total = 0;

        for (const auto &entry : staff.sources)
        {
            total += entry.count;
        }

        return total;
    }

    void setStaff(
        World &world, antwika::ecs::Entity entity, const Staff &staff)
    {
        if (world.has<Staff>(entity))
        {
            world.set<Staff>(entity, staff);
            return;
        }

        world.add<Staff>(entity, staff);
    }

}
