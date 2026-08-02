#include "antwika/game/StandingBuildings.hpp"

#include <cstdint>

#include "antwika/game/Building.hpp"
#include "antwika/game/Footprint.hpp"

namespace antwika::game
{

    StandingBuildings standingBuildings(const World &world)
    {
        StandingBuildings standing;

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto origin = world.get<Cell>(entity);
            const auto footprint =
                footprintOf(world.get<Building>(entity).kind);

            for (std::int32_t dy = 0; dy < footprint.height; ++dy)
            {
                for (std::int32_t dx = 0; dx < footprint.width; ++dx)
                {
                    standing.emplace(
                        Cell{.x = origin.x + dx, .y = origin.y + dy},
                        entity);
                }
            }
        }

        return standing;
        // The excluded line is the local map's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
