#include "antwika/game/RuinSystem.hpp"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <set>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        // As the crow flies, which is how a fireman answers a call.
        // He crosses open ground, so no road network can shorten it.
        [[nodiscard]] std::int64_t strides(Cell from, Cell to) noexcept
        {
            return std::abs(
                       static_cast<std::int64_t>(from.x)
                       - static_cast<std::int64_t>(to.x))
                + std::abs(
                       static_cast<std::int64_t>(from.y)
                       - static_cast<std::int64_t>(to.y));
        }

        // The free firemen in ascending (Cell, Entity) order.
        // Out of a set rather than a view, for deliver()'s reason.
        // A fire is a limited amount two of them contend over.
        // The busy set covers the already-called.
        // Seeded from the committed calls, it grows with this tick's.
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

        // The nearest free fireman to one fire, if any is free.
        // Strictly nearer wins, so a tie keeps the earlier candidate.
        // The candidates arrive in ascending (Cell, Entity) order.
        // Which is what makes "the earlier one" a total answer.
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
    } // namespace

    void RuinSystem::update(World &world, antwika::time::Tick)
    {
        // The fires in ascending Cell order, and who is coming.
        // A view's order is nobody's -- see AllocationOrderTest.
        std::map<Cell, Entity> burning;
        std::set<Entity> attended;
        std::set<Entity> busy;

        for (const auto entity : world.view<Ruin, Cell>())
        {
            auto ruin = world.get<Ruin>(entity);

            if (ruin.state != RuinState::Burning)
            {
                continue;
            }

            // The fire burning down is idempotent per ruin.
            // So it may run off the view while dispatch may not.
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
            }
        }

        if (burning.empty())
        {
            return;
        }

        // Who is already answering one, so no fire is answered twice.
        // And no fireman is sent to two.
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
                // Nobody free; the fire burns until somebody is.
                continue;
            }

            world.add<FireCall>(fireman, FireCall{.target = entity});
            std::erase_if(
                free,
                [fireman](const auto &candidate)
                { return candidate.second == fireman; });
        }
    }

} // namespace antwika::game
