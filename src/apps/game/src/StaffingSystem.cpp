#include "antwika/game/StaffingSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/StandingBuildings.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Workforce.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        // Why hire() never runs out of staff slots.
        // A full ledger holds at least as many people as any kind wants.
        // So an understaffed workplace always has a free slot.
        static_assert(
            []
            {
                for (std::size_t index = 0; index < kBuildingKindCount;
                     ++index)
                {
                    if (workersWantedBy(static_cast<BuildingKind>(index))
                        > static_cast<std::int32_t>(kMaxStaffSources))
                    {
                        return false;
                    }
                }

                return true;
            }(),
            "a full staff ledger must already satisfy any kind");

        // Seeded from the last commit the first time each is touched.
        // BuildingSystem's Pending, for the same accumulation reason.
        template <typename Component>
        [[nodiscard]] Component &touch(
            const World &world,
            std::map<Entity, Component> &pending,
            Entity entity)
        {
            const auto found = pending.find(entity);

            if (found != pending.end())
            {
                return found->second;
            }

            const auto seed = world.has<Component>(entity)
                ? world.get<Component>(entity)
                : Component{};

            return pending.emplace(entity, seed).first->second;
        }

        // Drop one person from the lowest occupied slot.
        // Slot order is arrival order, so this is total by construction.
        // Closed-up rather than left holed.
        // The lowest free slot is always past the last occupied one.
        // Answers whose person left, or nobody from an empty ledger.
        [[nodiscard]] Entity shedOne(Staff &staff)
        {
            for (auto &entry : staff.sources)
            {
                if (entry.count <= 0)
                {
                    continue;
                }

                const auto house = entry.house;
                --entry.count;

                if (entry.count == 0)
                {
                    entry.house = kNullEntity;
                }

                return house;
            }

            return kNullEntity;
        }

        void compact(Staff &staff)
        {
            std::size_t keep = 0;

            for (std::size_t slot = 0; slot < kMaxStaffSources; ++slot)
            {
                if (staff.sources[slot].count > 0)
                {
                    staff.sources[keep++] = staff.sources[slot];
                }
            }

            for (; keep < kMaxStaffSources; ++keep)
            {
                staff.sources[keep] = StaffEntry{};
            }
        }

        void compact(Employment &employment)
        {
            std::size_t keep = 0;

            for (std::size_t slot = 0; slot < kMaxJobs; ++slot)
            {
                if (employment.jobs[slot].count > 0)
                {
                    employment.jobs[keep++] = employment.jobs[slot];
                }
            }

            for (; keep < kMaxJobs; ++keep)
            {
                employment.jobs[keep] = JobHolding{};
            }
        }

        // Take one person off a house's holding at one workplace.
        void releaseJob(
            Employment &employment, Entity workplace)
        {
            for (auto &holding : employment.jobs)
            {
                // A matching holding always counts somebody.
                // compact() clears an emptied one on every write.
                if (holding.workplace != workplace)
                {
                    continue;
                }

                --holding.count;

                if (holding.count == 0)
                {
                    holding.workplace = kNullEntity;
                }

                compact(employment);
                return;
            }
        }

        // Credit one hiring on both ledgers, if both have room.
        // Answers how many actually changed hands.
        [[nodiscard]] std::int32_t hire(
            Staff &staff,
            Employment &employment,
            Entity workplace,
            Entity house,
            std::int32_t people)
        {
            StaffEntry *entry = nullptr;

            for (auto &candidate : staff.sources)
            {
                if (candidate.house == house
                    || (entry == nullptr
                        && candidate.house == kNullEntity))
                {
                    entry = &candidate;

                    if (candidate.house == house)
                    {
                        break;
                    }
                }
            }

            JobHolding *holding = nullptr;

            for (auto &candidate : employment.jobs)
            {
                if (candidate.workplace == workplace
                    || (holding == nullptr
                        && candidate.workplace == kNullEntity))
                {
                    holding = &candidate;

                    if (candidate.workplace == workplace)
                    {
                        break;
                    }
                }
            }

            // A full house-side ledger hires nobody.
            // The walker keeps its load for the next workplace.
            // The staff side always has a slot.
            // See the assert on kMaxStaffSources below the includes.
            if (holding == nullptr)
            {
                return 0;
            }

            entry->house = house;
            entry->count += people;
            holding->workplace = workplace;
            holding->count += people;
            return people;
        }
    } // namespace

    StaffingSystem::StaffingSystem(Tuning tuning) : tuning(tuning)
    {
    }

    void StaffingSystem::update(World &world, antwika::time::Tick)
    {
        std::map<Entity, Staff> staffs;
        std::map<Entity, Employment> employments;
        std::map<Entity, Walker> walkers;

        // Pass one: tend.
        // Ledgers exist for every workplace, and name only the living.
        for (const auto entity : world.view<Building, Cell>())
        {
            const auto kind = world.get<Building>(entity).kind;

            if (workersWantedBy(kind) <= 0)
            {
                continue;
            }

            auto &staff = touch(world, staffs, entity);

            for (auto &entry : staff.sources)
            {
                if (entry.house != kNullEntity
                    && !world.alive(entry.house))
                {
                    entry = StaffEntry{};
                }
            }

            compact(staff);
        }

        for (const auto entity : world.view<Employment>())
        {
            auto &employment = touch(world, employments, entity);

            for (auto &holding : employment.jobs)
            {
                if (holding.workplace != kNullEntity
                    && !world.alive(holding.workplace))
                {
                    holding = JobHolding{};
                }
            }

            compact(employment);
        }

        // Pass two: decay.
        // One person drifts home per period, from the lowest slot.
        for (auto &[entity, staff] : staffs)
        {
            if (staff.ticksUntilDecay > 0)
            {
                --staff.ticksUntilDecay;
                continue;
            }

            staff.ticksUntilDecay = tuning.staffDecayPeriodTicks;

            const auto house = shedOne(staff);
            compact(staff);

            // Alive by construction.
            // The tend pass has dropped the dead this very tick.
            if (house != kNullEntity)
            {
                releaseJob(
                    touch(world, employments, house), entity);
            }
        }

        // Pass three: transfer.
        // Ascending (Cell, Entity), out of a set rather than a view.
        // Workforce is a limited amount two labourers may contend for.
        const auto standing = standingBuildings(world);
        std::set<std::pair<Cell, Entity>> movers;

        for (const auto entity : world.view<Walker, Cell>())
        {
            if (world.get<Walker>(entity).kind == WalkerKind::Labourer)
            {
                movers.emplace(world.get<Cell>(entity), entity);
            }
        }

        for (const auto &[at, entity] : movers)
        {
            auto &walker = touch(world, walkers, entity);

            for (std::size_t index = 0; index < kDirectionCount; ++index)
            {
                if (walker.carried <= 0)
                {
                    break;
                }

                const auto beside =
                    standing.find(step(at, static_cast<Direction>(index)));

                if (beside == standing.end())
                {
                    continue;
                }

                const auto workplace = beside->second;
                auto &staff = touch(world, staffs, workplace);

                const auto wanted = workersWantedBy(
                    world.get<Building>(workplace).kind);
                const auto need = wanted - staffCount(staff);

                if (need <= 0 || !world.alive(walker.home))
                {
                    continue;
                }

                const auto given = hire(
                    staff,
                    touch(world, employments, walker.home),
                    workplace,
                    walker.home,
                    std::min(need, walker.carried));

                walker.carried -= given;
            }

            // Out of people, so the errand is over: turn for home.
            // WalkerSystem reads a spent budget as exactly that.
            if (walker.carried <= 0 && walker.stepsUntilHome > 0)
            {
                walker.stepsUntilHome = 0;
            }
        }

        for (const auto &[entity, staff] : staffs)
        {
            setStaff(world, entity, staff);
        }

        for (const auto &[entity, employment] : employments)
        {
            setEmployment(world, entity, employment);
        }

        for (const auto &[entity, walker] : walkers)
        {
            world.set<Walker>(entity, walker);
        }
    }

} // namespace antwika::game
