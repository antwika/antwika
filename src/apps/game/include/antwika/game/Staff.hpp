#pragma once

#include <array>
#include <vector>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief How many houses may staff one workplace at once.
     *
     * A fixed array rather than a vector, since ecs::Component wants a
     * trivially copyable, standard-layout type -- Building::walkers'
     * reason exactly. Four is more houses than any kind wants workers,
     * so the cap can only bind where many houses each sent one person.
     */
    inline constexpr std::size_t kMaxStaffSources = 4;

    /**
     * @brief One house's people at one workplace.
     */
    struct StaffEntry
    {
        /** @brief The house they live in; kNullEntity in a free slot. */
        antwika::ecs::Entity house = antwika::ecs::kNullEntity;

        /** @brief How many of its people work here. */
        std::int32_t count = 0;

        [[nodiscard]] bool operator==(const StaffEntry &other) const
            = default;
    };

    /**
     * @brief Who works at a building, and where they live.
     *
     * **The ledger both readouts hang off**: staffingOf() sums it for
     * "Staff 6/8", and the house half of every entry is what lets a
     * decay hand the person back to the household that supplied them.
     * A bare count could say how staffed a building is; it could never
     * say whose people leave when the staffing runs down.
     *
     * **An absent component still means fully staffed**, the value the
     * game had before labour walked -- which is what keeps every
     * fixture that runs no staffing systems meaning what it meant.
     * StaffingSystem gives every workplace an empty one on its first
     * tick, so under the real composition the ledger rules at once.
     *
     * A slot number is not a role: a house goes into the lowest free
     * slot, and a decay takes from the lowest occupied one, so two
     * buildings staffed by the same houses in the same order hold and
     * shed them identically.
     */
    struct Staff
    {
        /** @brief Which houses staff it, in arrival order. */
        std::array<StaffEntry, kMaxStaffSources> sources{};

        /**
         * @brief Ticks until one person drifts back home.
         *
         * Per building rather than a shared period, for the reason
         * every countdown here is: two workplaces staffed a tick apart
         * must not shed in lockstep for ever.
         */
        std::int32_t ticksUntilDecay = 0;

        [[nodiscard]] bool operator==(const Staff &other) const = default;
    };

    /**
     * @brief How long a workplace keeps somebody before one drifts off.
     */
    inline constexpr std::int32_t kStaffDecayPeriodTicks = 500;

    /**
     * @brief One house's people at one workplace, by index.
     */
    struct StoredStaffEntry
    {
        /** @brief Which stored building they live in, by index. */
        std::size_t house = 0;

        /** @brief How many of its people work here. */
        std::int32_t count = 0;

        [[nodiscard]] bool operator==(
            const StoredStaffEntry &other) const = default;
    };

    /**
     * @brief A workplace's staff ledger, as a put-away city holds it.
     */
    struct StoredStaff
    {
        /** @brief Every occupied entry, in slot order. */
        std::vector<StoredStaffEntry> entries;

        /** @brief Ticks until one person drifts home. */
        std::int32_t ticksUntilDecay = 0;

        [[nodiscard]] bool operator==(const StoredStaff &other) const
            = default;
    };

    /**
     * @brief Sum a ledger's people.
     * @param staff The ledger to sum.
     * @return How many people work there, never negative.
     */
    [[nodiscard]] std::int32_t staffCount(const Staff &staff);

    /**
     * @brief Write a building's staff, whether or not it had any.
     *
     * setCoverage()'s counterpart, for its reason exactly: World::add()
     * is staged and World::set() refuses an entity that has no such
     * component yet, so "add or set" is a decision every writer would
     * otherwise make for itself.
     *
     * @param world Staged into; the write lands at the next commit().
     * @param entity The workplace to write; must be alive.
     * @param staff The ledger to write.
     */
    void setStaff(
        World &world, antwika::ecs::Entity entity, const Staff &staff);

} // namespace antwika::game
