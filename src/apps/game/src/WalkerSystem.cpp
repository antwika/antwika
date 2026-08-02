#include "antwika/game/WalkerSystem.hpp"

#include <cstdint>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
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
            const auto bound = errandTargetOf(world, entity);

            // An errand is the one thing that overrides roaming.
            // A walker without one keeps every rule it had before.
            // Which is why the two arms below are untouched.
            // An errand naming nowhere is a load on the rounds.
            if (bound != antwika::ecs::kNullEntity)
            {
                runErrand(world, entity, walker, at, bound);
                continue;
            }

            if (walker.stepsUntilHome > 0)
            {
                roam(world, entity, walker, at);
                continue;
            }

            headHome(world, entity, walker, at);
        }
    }

    void WalkerSystem::runErrand(
        World &world,
        antwika::ecs::Entity entity,
        const Walker &walker,
        Cell at,
        antwika::ecs::Entity bound)
    {
        // A dead target has no Cell, so this is one lookup.
        // It is the same arm headHome() uses, and for its reason.
        // A destination demolished, a road pulled out, no building.
        // All three are answered by the walker being gone.
        // And none of them is an error.
        if (!world.has<Cell>(bound))
        {
            world.destroy(entity);
            return;
        }

        const auto door = world.get<Cell>(bound);
        const auto footprint =
            footprintOf(world.get<Building>(bound).kind);
        const auto heading =
            stepTowards(at, door, footprint, paths, extent);

        if (!heading.has_value())
        {
            world.destroy(entity);
            return;
        }

        const auto onto = step(at, *heading);

        if (covers(door, footprint, onto))
        {
            // Arriving is the one place the two legs differ.
            // On the way back there is nothing left to do.
            // So the walker is gone, as one that walked home is.
            // On the way out it stands where it is.
            // Whoever gave the errand decides what happens next.
            // And that system runs in a later phase.
            // So stepping on here would destroy it too soon.
            if (world.get<Errand>(entity).leg == ErrandLeg::Returning)
            {
                world.destroy(entity);
            }

            return;
        }

        auto moved = walker;
        moved.facing = *heading;
        moved.ticksUntilStep = kTicksPerStep - 1;
        moved.from = at;

        world.set<Walker>(entity, moved);
        world.set<Cell>(entity, onto);
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
        const auto footprint =
            footprintOf(world.get<Building>(walker.home).kind);
        const auto heading =
            stepTowards(at, door, footprint, paths, extent);

        // Walled off, or the road under it has gone.
        // Either way its budget is spent and there is nowhere to go.
        if (!heading.has_value())
        {
            world.destroy(entity);
            return;
        }

        const auto onto = step(at, *heading);

        // Arriving is stepping onto the building itself.
        // Any cell of its block will do.
        // The whole thing is what the walker was heading for.
        if (covers(door, footprint, onto))
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
