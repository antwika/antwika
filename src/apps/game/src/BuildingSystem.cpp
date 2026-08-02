#include "antwika/game/BuildingSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        using Standing = std::map<Cell, Entity>;
        using Pending = std::map<Entity, Building>;

        // Seeded from the last commit the first time it is touched.
        // So every change this tick accumulates onto one value.
        [[nodiscard]] Building &touch(
            const World &world, Pending &pending, Entity entity)
        {
            const auto found = pending.find(entity);

            if (found != pending.end())
            {
                return found->second;
            }

            return pending.emplace(entity, world.get<Building>(entity))
                .first->second;
        }

        // What ends a building: bad luck, or an empty larder.
        // A source holds stock nobody drains, so only risk takes one.
        // And only what sustains() names is a larder.
        // A house holding no clay is a house nobody has carted to yet.
        [[nodiscard]] bool isLost(const Building &building)
        {
            if (building.risk >= kMaxRisk)
            {
                return true;
            }

            if (!consumes(building.kind))
            {
                return false;
            }

            return std::ranges::any_of(
                kResources,
                [&building](Resource resource)
                {
                    return sustains(resource)
                        && building.stock[resourceIndex(resource)] <= 0;
                });
        }

        void deliverTo(
            Building &building, Walker &walker, Resource resource)
        {
            auto &held = building.stock[resourceIndex(resource)];
            const auto room = capacityOf(building.kind) - held;
            const auto given = std::min(walker.carried, room);

            if (given <= 0)
            {
                return;
            }

            held += given;
            walker.carried -= given;
        }

        // Every building beside a cell, in ascending Cell order.
        // The four neighbours are a fixed set, so this is total.
        // Direction order is total too.
        // It is not an order over anything a reader can name.
        //
        // No guard against serving one building twice.
        // A rectangle cannot be touched twice from one outside cell.
        // Two of a cell's neighbours in it would put the cell in it.
        // An opposite pair spans the cell in one axis.
        // A perpendicular pair spans it in both.
        // So it would be a road under a building, which nothing places.
        [[nodiscard]] Standing neighboursOf(
            const Standing &standing, Cell at)
        {
            Standing beside;

            for (std::size_t index = 0; index < kDirectionCount; ++index)
            {
                const auto cell = step(at, static_cast<Direction>(index));
                const auto found = standing.find(cell);

                if (found != standing.end())
                {
                    beside.emplace(cell, found->second);
                }
            }

            return beside;
        } // GCOVR_EXCL_LINE

        void deliver(World &world, const Standing &standing, Pending &pending)
        {
            // Ascending (Cell, Entity), out of a set not a view.
            // Two walkers filling one shelf split what room it has.
            // deliverTo() clamps, so the first of them gets the last.
            // The entity is in the key since walkers may share a cell.
            std::set<std::pair<Cell, Entity>> movers;

            for (const auto entity : world.view<Walker, Cell>())
            {
                movers.emplace(world.get<Cell>(entity), entity);
            }

            for (const auto &[at, entity] : movers)
            {
                auto walker = world.get<Walker>(entity);
                const auto errand = world.has<Errand>(entity)
                    ? std::optional<Errand>(world.get<Errand>(entity))
                    : std::nullopt;

                // A walker on its way back hands nothing over here.
                // A load that never changed hands is its sender's.
                // That system credits its own building in a later phase.
                // Which is the one place the cadence cannot undo it.
                // See acceptsAt() for what undoes it here.
                if (errand.has_value()
                    && errand->leg == ErrandLeg::Returning)
                {
                    continue;
                }

                const auto carries = errand.has_value()
                    ? std::optional<Resource>(errand->carrying)
                    : carriedResource(walker.kind);
                const auto bound = errand.has_value() ? errand->destination
                                                      : kNullEntity;
                const auto before = walker.carried;

                for (const auto &[cell, standingHere] :
                     neighboursOf(standing, at))
                {
                    // An errand is bound to one building.
                    // A cart must not shed its load along the way.
                    if (bound != kNullEntity && standingHere != bound)
                    {
                        continue;
                    }

                    auto &building = touch(world, pending, standingHere);

                    if (!carries.has_value())
                    {
                        // A walker whose kind carries nothing fixed.
                        // What it does instead is take risk off.
                        // Which is coverage in disguise.
                        building.risk =
                            std::max(0, building.risk - kRiskRelief);
                        continue;
                    }

                    // A load bound nowhere goes to whoever eats it.
                    // A seller filling a storehouse undoes a buyer.
                    // And one filling its own market moves nothing.
                    if (bound == kNullEntity && !consumes(building.kind))
                    {
                        continue;
                    }

                    deliverTo(building, walker, *carries);
                }

                if (walker.carried != before)
                {
                    world.set<Walker>(entity, walker);
                }
            }
        }

        void age(World &world, Pending &pending)
        {
            for (const auto entity : world.view<Building, Cell>())
            {
                auto &building = touch(world, pending, entity);

                if (building.ticksUntilDrain > 0)
                {
                    --building.ticksUntilDrain;
                }
                else
                {
                    building.ticksUntilDrain = kDrainPeriodTicks;

                    // Only a house eats; a source keeps what it holds.
                    if (consumes(building.kind))
                    {
                        for (auto &held : building.stock)
                        {
                            held = std::max(0, held - 1);
                        }
                    }
                }

                if (building.ticksUntilRisk > 0)
                {
                    --building.ticksUntilRisk;
                    continue;
                }

                building.ticksUntilRisk = kRiskPeriodTicks;
                building.risk = std::min(kMaxRisk, building.risk + 1);
            }
        }
    } // namespace

    BuildingSystem::BuildingSystem(BuildingIndex &built) : built(built)
    {
    }

    void BuildingSystem::update(World &world, antwika::time::Tick)
    {
        // Where each building is, so a neighbour is a lookup.
        // Rather than a scan of every building for every walker.
        Standing standing;

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto origin = world.get<Cell>(entity);
            const auto footprint =
                footprintOf(world.get<Building>(entity).kind);

            // Keyed by every cell the block stands on.
            // So a walker beside its far corner finds it.
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

        Pending pending;

        deliver(world, standing, pending);
        age(world, pending);

        for (const auto &[entity, building] : pending)
        {
            if (isLost(building))
            {
                built.erase(
                    world.get<Cell>(entity), footprintOf(building.kind));
                world.destroy(entity);
                continue;
            }

            world.set<Building>(entity, building);
        }
    }

} // namespace antwika::game
