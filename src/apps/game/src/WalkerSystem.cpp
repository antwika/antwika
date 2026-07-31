#include "antwika/game/WalkerSystem.hpp"

#include <cstdint>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Homing.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    WalkerSystem::WalkerSystem(const PathIndex &paths, GridExtent extent)
        : paths(paths), extent(extent)
    {
    }

    void WalkerSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto walker = world.get<Walker>(entity);

            if (walker.ticksUntilStep > 0)
            {
                // Copied and adjusted rather than rebuilt.
                // So a member added later is carried across.
                // Rebuilding this once dropped `from` entirely.
                auto waiting = walker;
                waiting.ticksUntilStep =
                    static_cast<std::uint8_t>(walker.ticksUntilStep - 1);
                world.set<Walker>(entity, waiting);
                continue;
            }

            const auto at = world.get<Cell>(entity);

            if (walker.stepsUntilHome > 0)
            {
                roam(world, entity, walker, at);
                continue;
            }

            headHome(world, entity, walker, at);
        }
    }

    void WalkerSystem::roam(
        World &world,
        antwika::ecs::Entity entity,
        const Walker &walker,
        Cell at)
    {
        const auto heading =
            nextFacing(walker.facing, paths.neighboursOf(at));

        if (!heading.has_value())
        {
            // Nowhere to go, so it keeps what it last came from.
            // A renderer then draws it standing still.
            // It does not tire either, since it has not walked.
            return;
        }

        auto moved = walker;
        moved.facing = *heading;
        moved.ticksUntilStep = kTicksPerStep - 1;
        moved.from = at;
        moved.stepsUntilHome = walker.stepsUntilHome - 1;

        world.set<Walker>(entity, moved);
        world.set<Cell>(entity, step(at, *heading));
    }

    void WalkerSystem::headHome(
        World &world,
        antwika::ecs::Entity entity,
        const Walker &walker,
        Cell at)
    {
        // A dead home has no Cell, so this is one lookup.
        // Rather than a liveness test and a lookup.
        // It is also what puts a walker nobody sent on this arm.
        // Which is the whole point of there being one arm.
        if (!world.has<Cell>(walker.home))
        {
            world.destroy(entity);
            return;
        }

        const auto door = world.get<Cell>(walker.home);
        const auto heading = stepTowards(at, door, paths, extent);

        // Walled off, or the road under it has gone.
        // Either way its budget is spent and there is nowhere to go.
        if (!heading.has_value())
        {
            world.destroy(entity);
            return;
        }

        const auto onto = step(at, *heading);

        // Arriving is stepping onto the building itself.
        // Which is the one cell of the route that is not a road.
        if (onto == door)
        {
            world.destroy(entity);
            return;
        }

        auto moved = walker;
        moved.facing = *heading;
        moved.ticksUntilStep = kTicksPerStep - 1;
        moved.from = at;

        world.set<Walker>(entity, moved);
        world.set<Cell>(entity, onto);
    }

} // namespace antwika::game
