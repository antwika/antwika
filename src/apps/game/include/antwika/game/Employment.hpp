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
     * @brief How many workplaces one house's people may be spread over.
     *
     * Staff::kMaxStaffSources' mirror, fixed for the same trivially
     * copyable reason. One labourer staffs the buildings it walks past
     * in turn, so a house's people cluster at a few nearby workplaces
     * rather than scattering across the city.
     */
    inline constexpr std::size_t kMaxJobs = 4;

    /**
     * @brief One workplace some of a house's people work at.
     */
    struct JobHolding
    {
        /** @brief Where they work; kNullEntity in a free slot. */
        antwika::ecs::Entity workplace = antwika::ecs::kNullEntity;

        /** @brief How many of the house's people work there. */
        std::int32_t count = 0;

        [[nodiscard]] bool operator==(const JobHolding &other) const
            = default;
    };

    /**
     * @brief Which of a household's people are employed, and where.
     *
     * **Staff's mirror, seen from home.** The two ledgers say one fact
     * from two ends -- these people work there -- and StaffingSystem is
     * the only writer of either, which is what keeps them agreeing.
     * The house half is what "Unemployed 4/16" is answered from, and
     * it is why a fully employed house stops sending its walker.
     *
     * An absent component means nobody employed and a dispatch due,
     * which is what every house held before labour walked.
     */
    struct Employment
    {
        /** @brief Where the house's people work, in hiring order. */
        std::array<JobHolding, kMaxJobs> jobs{};

        /**
         * @brief Ticks until the house may send its next labourer.
         *
         * Per house, for every countdown's reason: two houses built a
         * tick apart must not dispatch in lockstep for ever.
         */
        std::int32_t ticksUntilDispatch = 0;

        [[nodiscard]] bool operator==(const Employment &other) const
            = default;
    };

    /**
     * @brief How often a house with idle hands sends its labourer out.
     */
    inline constexpr std::int32_t kLabourPeriodTicks = 150;

    /**
     * @brief One workplace some of a house's people work at, by index.
     */
    struct StoredJob
    {
        /** @brief Which stored building they work at, by index. */
        std::size_t workplace = 0;

        /** @brief How many of the house's people work there. */
        std::int32_t count = 0;

        [[nodiscard]] bool operator==(const StoredJob &other) const
            = default;
    };

    /**
     * @brief A house's employment ledger, as a put-away city holds it.
     */
    struct StoredEmployment
    {
        /** @brief Every occupied holding, in slot order. */
        std::vector<StoredJob> jobs;

        /** @brief Ticks until the house may send its next labourer. */
        std::int32_t ticksUntilDispatch = 0;

        [[nodiscard]] bool operator==(
            const StoredEmployment &other) const = default;
    };

    /**
     * @brief Sum a house's employed people.
     * @param employment The ledger to sum.
     * @return How many of its people hold a job, never negative.
     */
    [[nodiscard]] std::int32_t employedCount(const Employment &employment);

    /**
     * @brief Write a house's employment, whether or not it had any.
     *
     * setStaff()'s counterpart, on exactly its terms.
     *
     * @param world Staged into; the write lands at the next commit().
     * @param entity The house to write; must be alive.
     * @param employment The ledger to write.
     */
    void setEmployment(
        World &world,
        antwika::ecs::Entity entity,
        const Employment &employment);

} // namespace antwika::game
