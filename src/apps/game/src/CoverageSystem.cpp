#include "antwika/game/CoverageSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <map>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/ServiceWalk.hpp"
#include "antwika/game/StandingBuildings.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;

        using Pending = std::map<Entity, Coverage>;

        [[nodiscard]] Coverage &touch(
            const World &world, Pending &pending, Entity entity)
        {
            const auto found = pending.find(entity);

            if (found != pending.end())
            {
                return found->second;
            }

            return pending.emplace(entity, coverageOf(world, entity))
                .first->second;
        }

        void decay(const World &world, Pending &pending)
        {
            for (const auto entity : world.view<Coverage>())
            {
                auto &coverage = touch(world, pending, entity);

                for (auto &left : coverage.ticksLeft)
                {
                    left = std::max(0, left - 1);
                }
            }
        }

        void refresh(
            const World &world,
            const StandingBuildings &standing,
            Pending &pending)
        {
            for (const auto entity : world.view<Walker, Cell>())
            {
                const auto confers =
                    serviceConferredBy(world.get<Walker>(entity).kind);

                if (!confers.has_value())
                {
                    continue;
                }

                const auto at = world.get<Cell>(entity);

                for (std::size_t index = 0; index < kDirectionCount;
                     ++index)
                {
                    const auto beside =
                        step(at, static_cast<Direction>(index));
                    const auto found = standing.find(beside);

                    if (found == standing.end())
                    {
                        continue;
                    }

                    auto &left = touch(world, pending, found->second)
                                     .ticksLeft[serviceIndex(*confers)];

                    left = std::max(left, kCoverageFull);
                }
            }
        }
    }

    void CoverageSystem::update(World &world, antwika::time::Tick)
    {
        const auto standing = standingBuildings(world);

        Pending pending;

        decay(world, pending);
        refresh(world, standing, pending);

        for (const auto &[entity, coverage] : pending)
        {
            setCoverage(world, entity, coverage);
        }
    }

}
