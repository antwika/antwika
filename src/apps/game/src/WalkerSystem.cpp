#include "antwika/game/WalkerSystem.hpp"

#include <cstdint>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    WalkerSystem::WalkerSystem(const PathIndex &paths) : paths(paths)
    {
    }

    void WalkerSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto walker = world.get<Walker>(entity);

            if (walker.ticksUntilStep > 0)
            {
                world.set<Walker>(
                    entity,
                    Walker{
                        .facing = walker.facing,
                        .ticksUntilStep = static_cast<std::uint8_t>(
                            walker.ticksUntilStep - 1)});
                continue;
            }

            const auto at = world.get<Cell>(entity);

            const auto heading =
                nextFacing(walker.facing, paths.neighboursOf(at));

            if (!heading.has_value())
            {
                continue;
            }

            world.set<Walker>(
                entity,
                Walker{
                    .facing = *heading,
                    .ticksUntilStep = kTicksPerStep - 1});
            world.set<Cell>(entity, step(at, *heading));
        }
    }

} // namespace antwika::game
