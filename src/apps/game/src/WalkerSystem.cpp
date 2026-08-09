#include "antwika/game/WalkerSystem.hpp"

#include <cstdint>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Homing.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    WalkerSystem::WalkerSystem(
        const PathIndex &paths,
        const BuildingIndex &built,
        GridExtent extent)
        : paths(paths), built(built), extent(extent)
    {
    }

    void WalkerSystem::update(World &world, antwika::time::Tick tick)
    {
        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto walker = world.get<Walker>(entity);

            if (walker.ticksUntilStep > 0)
            {
                auto waiting = walker;
                waiting.ticksUntilStep =
                    static_cast<std::uint8_t>(walker.ticksUntilStep - 1);
                world.set<Walker>(entity, waiting);
                continue;
            }

            const auto at = world.get<Cell>(entity);

            if (world.has<FireCall>(entity))
            {
                respond(world, entity, walker, at);
                continue;
            }

            if (world.has<Journey>(entity))
            {
                travel(world, entity, walker, at);
                continue;
            }

            const auto bound = errandTargetOf(world, entity);

            if (bound != antwika::ecs::kNullEntity)
            {
                runErrand(world, entity, walker, at, bound);
                continue;
            }

            if (walker.stepsUntilHome > 0)
            {
                roam(world, entity, walker, at, tick);
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

    void WalkerSystem::respond(
        World &world,
        antwika::ecs::Entity entity,
        const Walker &walker,
        Cell at)
    {
        const auto target = world.get<FireCall>(entity).target;

        if (!world.has<Ruin>(target)
            || world.get<Ruin>(target).state != RuinState::Burning)
        {
            world.destroy(entity);
            return;
        }

        const auto door = world.get<Cell>(target);
        const auto footprint =
            footprintOf(world.get<Ruin>(target).kind);

        const auto heading =
            stepAcross(at, door, footprint, built, extent);

        if (!heading.has_value())
        {
            world.destroy(entity);
            return;
        }

        const auto onto = step(at, *heading);

        if (covers(door, footprint, onto))
        {
            auto ruin = world.get<Ruin>(target);
            ruin.state = RuinState::Debris;
            ruin.ticksUntilOut = 0;
            world.set<Ruin>(target, ruin);

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

    void WalkerSystem::travel(
        World &world,
        antwika::ecs::Entity entity,
        const Walker &walker,
        Cell at)
    {
        const auto journey = world.get<Journey>(entity);
        const auto joining = journey.house != antwika::ecs::kNullEntity;

        if (joining && !world.has<Cell>(journey.house))
        {
            world.destroy(entity);
            return;
        }

        const auto footprint = joining
            ? footprintOf(world.get<Building>(journey.house).kind)
            : Footprint{.width = 1, .height = 1};

        const auto heading =
            stepAcross(at, journey.towards, footprint, built, extent);

        if (!heading.has_value())
        {
            world.destroy(entity);
            return;
        }

        const auto onto = step(at, *heading);

        if (covers(journey.towards, footprint, onto))
        {
            if (!joining)
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
        Cell at,
        antwika::time::Tick tick)
    {
        const auto heading = nextFacing(
            walker.facing,
            paths.neighboursOf(at),
            wanderRoll(tick, at, walker.facing));

        if (!heading.has_value())
        {
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

        if (!heading.has_value())
        {
            world.destroy(entity);
            return;
        }

        const auto onto = step(at, *heading);

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

}
