#include "antwika/game/BuildingSystem.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Demolition.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/StandingBuildings.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

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

        // What sets a building alight: risk run all the way up.
        // Which is what a district nobody serves comes to.
        [[nodiscard]] bool catchesFire(const Building &building)
        {
            return building.risk >= kMaxRisk;
        }

        // What ends a building outright: an empty larder.
        // Only what sustains() names is a larder.
        // A house holding no clay is a house nobody has carted to yet.
        // A source holds stock nobody drains, so it never starves.
        [[nodiscard]] bool starves(const Building &building)
        {
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
        [[nodiscard]] StandingBuildings neighboursOf(
            const StandingBuildings &standing, Cell at)
        {
            StandingBuildings beside;

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

        void deliver(
            World &world,
            const StandingBuildings &standing,
            Pending &pending)
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

                // An errand names what it carries; a kind otherwise does.
                const auto carries = errand.has_value()
                    ? std::optional<Resource>(errand->carrying)
                    : carriedResource(walker.kind);

                // A walker carrying nothing fixed hands nothing over.
                // It used to take risk off instead.
                // Which was coverage said as a subtraction.
                // It refreshes coverage now, in CoverageSystem.
                // So a delivery has nothing to do with one at all.
                if (!carries.has_value())
                {
                    continue;
                }

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

        // What holds a building's risk off, and the reason those two.
        // Safety is the fireman's and Structure is the engineer's.
        // Both used to relieve risk by walking past a building.
        // So the two services are exactly the old special cases.
        // Water and Health reach a house for what a house does with them.
        // Neither has anything to say about the building falling down.
        constexpr std::array<Service, 2> kRiskHeldOffBy{
            Service::Safety, Service::Structure};

        // Read as of the last commit.
        // Which is what the "serve" phase left at the previous tick.
        // One tick out of five hundred on a "came recently" countdown.
        [[nodiscard]] bool unserved(const World &world, Entity entity)
        {
            return std::ranges::any_of(
                kRiskHeldOffBy,
                [&world, entity](Service service)
                { return coverageOf(world, entity, service) <= 0; });
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

                // **The one thing that changed about risk.**
                // It used to rise here whatever was standing nearby.
                // And a passing fireman subtracted a lump from it.
                // Now it rises where the district is unserved.
                // And falls back where a walker keeps reaching it.
                // So a fire station is a thing a district has.
                // Rather than a thing that visits it.
                building.ticksUntilRisk = kRiskPeriodTicks;
                building.risk = unserved(world, entity)
                    ? std::min(kMaxRisk, building.risk + 1)
                    : std::max(0, building.risk - 1);
            }
        }
    } // namespace

    BuildingSystem::BuildingSystem(BuildingIndex &built, GridExtent extent)
        : built(built), extent(extent)
    {
    }

    void BuildingSystem::update(World &world, antwika::time::Tick)
    {
        // Where each building is, so a neighbour is a lookup.
        // Rather than a scan of every building for every walker.
        const auto standing = standingBuildings(world);

        Pending pending;

        deliver(world, standing, pending);
        age(world, pending);

        // Ascending Cell rather than the pending map's Entity order.
        // Both endings turn people out, and people are contended.
        // The walker limit and a vacancy's beds are split amounts.
        // An entity order is one a restore may renumber.
        // See AllocationOrderTest.
        // A cell is unique per building, so no tie-break is needed.
        //
        // One map for both endings rather than one map each.
        // So a fire and a starvation resolve in one Cell order too.
        // Fire is asked first.
        // A building starving the tick its risk maxes therefore burns.
        // One answer, stated by the order of the two tests.
        // Rather than by a tie rule written beside them.
        std::map<Cell, std::pair<Entity, bool>> lost;

        for (const auto &[entity, building] : pending)
        {
            if (catchesFire(building))
            {
                lost.emplace(
                    world.get<Cell>(entity),
                    std::make_pair(entity, true));
                continue;
            }

            if (starves(building))
            {
                lost.emplace(
                    world.get<Cell>(entity),
                    std::make_pair(entity, false));
                continue;
            }

            world.set<Building>(entity, building);
        }

        for (const auto &[at, ending] : lost)
        {
            if (ending.second)
            {
                ignite(world, built, ending.first, extent);
            }
            else
            {
                demolish(world, built, ending.first, extent);
            }
        }
    }

} // namespace antwika::game
