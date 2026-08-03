#include "antwika/game/LabourQuery.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/Workforce.hpp"

namespace antwika::game
{

    Staffing staffingOf(const World &world, antwika::ecs::Entity entity)
    {
        const auto wanted = world.has<Building>(entity)
            ? workersWantedBy(world.get<Building>(entity).kind)
            : 0;

        // Fully staffed rather than empty -- see the header.
        // A workplace the staffing has not reached behaves as before.
        // Which is to say as it did when nobody lived here at all.
        if (!world.has<Staff>(entity))
        {
            return Staffing{.filled = wanted, .wanted = wanted};
        }

        return Staffing{
            .filled = staffCount(world.get<Staff>(entity)),
            .wanted = wanted};
    }

    std::int32_t workersAt(const World &world, antwika::ecs::Entity entity)
    {
        return staffingOf(world, entity).filled;
    }

} // namespace antwika::game
