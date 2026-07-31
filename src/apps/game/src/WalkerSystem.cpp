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
                // The whole walker is rebuilt here rather than nudged.
                // So `from` has to be carried across by hand.
                // Dropping it would snap a walker back every other tick.
                world.set<Walker>(
                    entity,
                    Walker{
                        .facing = walker.facing,
                        .ticksUntilStep = static_cast<std::uint8_t>(
                            walker.ticksUntilStep - 1),
                        .from = walker.from});
                continue;
            }

            const auto at = world.get<Cell>(entity);

            const auto heading =
                nextFacing(walker.facing, paths.neighboursOf(at));

            if (!heading.has_value())
            {
                // Nowhere to go, so it keeps what it last came from.
                // A renderer then draws it standing still.
                continue;
            }

            world.set<Walker>(
                entity,
                Walker{
                    .facing = *heading,
                    .ticksUntilStep = kTicksPerStep - 1,
                    .from = at});
            world.set<Cell>(entity, step(at, *heading));
        }
    }

} // namespace antwika::game
