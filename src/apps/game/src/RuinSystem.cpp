#include "antwika/game/RuinSystem.hpp"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Demolition.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        [[nodiscard]] std::int64_t strides(Cell from, Cell to) noexcept
        {
            return std::abs(
                       static_cast<std::int64_t>(from.x)
                       - static_cast<std::int64_t>(to.x))
                + std::abs(
                       static_cast<std::int64_t>(from.y)
                       - static_cast<std::int64_t>(to.y));
        }

        [[nodiscard]] std::set<std::pair<Cell, Entity>> freeFiremen(
            const World &world, const std::set<Entity> &busy)
        {
            std::set<std::pair<Cell, Entity>> free;

            for (const auto entity : world.view<Walker, Cell>())
            {
                if (world.get<Walker>(entity).kind == WalkerKind::Fireman
                    && !busy.contains(entity))
                {
                    free.emplace(world.get<Cell>(entity), entity);
                }
            }

            return free;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool touches(
            const Cell fire,
            const Footprint burning,
            const Cell at,
            const Footprint standing) noexcept
        {
            return fire.x - 1 < at.x + standing.width
                && at.x < fire.x + burning.width + 1
                && fire.y - 1 < at.y + standing.height
                && at.y < fire.y + burning.height + 1;
        }

        [[nodiscard]] Entity nearestFireman(
            const std::set<std::pair<Cell, Entity>> &free, Cell fire)
        {
            auto best = kNullEntity;
            std::int64_t bestCost = 0;

            for (const auto &[at, entity] : free)
            {
                const auto cost = strides(at, fire);

                if (best == kNullEntity || cost < bestCost)
                {
                    best = entity;
                    bestCost = cost;
                }
            }

            return best;
        }
    }

    RuinSystem::RuinSystem(
        BuildingIndex &built,
        const GridExtent extent,
        GameConfig config) noexcept
        : built(built), extent(extent), config(config)
    {
    }

    void RuinSystem::spread(World &world, const Cell at, const Ruin &ruin)
    {
        const auto burningFootprint = footprintOf(ruin.kind);

        std::vector<Entity> caught;

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto standing = world.get<Cell>(entity);

            if (touches(
                    at,
                    burningFootprint,
                    standing,
                    footprintOf(world.get<Building>(entity).kind)))
            {
                caught.push_back(entity);
            }
        }

        for (const auto entity : caught)
        {
            ignite(world, built, entity, extent, config);
        }
    }

    void RuinSystem::update(World &world, antwika::time::Tick)
    {
        std::map<Cell, Entity> burning;
        std::map<Cell, Ruin> spreading;
        std::set<Entity> attended;
        std::set<Entity> busy;

        for (const auto entity : world.view<Ruin, Cell>())
        {
            auto ruin = world.get<Ruin>(entity);

            if (ruin.state != RuinState::Burning)
            {
                continue;
            }

            if (ruin.ticksUntilOut > 0)
            {
                --ruin.ticksUntilOut;
            }
            else
            {
                ruin.state = RuinState::Debris;
            }

            world.set<Ruin>(entity, ruin);

            if (ruin.state == RuinState::Burning)
            {
                burning.emplace(world.get<Cell>(entity), entity);

                if (config.burnDurationTicks - ruin.ticksUntilOut
                    >= config.spreadDelayTicks)
                {
                    spreading.emplace(world.get<Cell>(entity), ruin);
                }
            }
        }

        for (const auto &[at, ruin] : spreading)
        {
            spread(world, at, ruin);
        }

        if (burning.empty())
        {
            return;
        }

        for (const auto entity : world.view<FireCall>())
        {
            attended.insert(world.get<FireCall>(entity).target);
            busy.insert(entity);
        }

        auto free = freeFiremen(world, busy);

        for (const auto &[at, entity] : burning)
        {
            if (attended.contains(entity))
            {
                continue;
            }

            const auto fireman = nearestFireman(free, at);

            if (fireman == kNullEntity)
            {
                continue;
            }

            world.add<FireCall>(fireman, FireCall{.target = entity});
            std::erase_if(
                free,
                [fireman](const auto &candidate)
                { return candidate.second == fireman; });
        }
    }

}
