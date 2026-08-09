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

    inline constexpr std::size_t kMaxJobs = 4;

    struct JobHolding final
    {
        antwika::ecs::Entity workplace = antwika::ecs::kNullEntity;

        std::int32_t count = 0;

        [[nodiscard]] bool operator==(const JobHolding &other) const
            = default;
    };

    struct Employment final
    {
        std::array<JobHolding, kMaxJobs> jobs{};

        std::int32_t ticksUntilDispatch = 0;

        [[nodiscard]] bool operator==(const Employment &other) const
            = default;
    };

    inline constexpr std::int32_t kLabourPeriodTicks = 150;

    struct StoredJob final
    {
        std::size_t workplace = 0;

        std::int32_t count = 0;

        [[nodiscard]] bool operator==(const StoredJob &other) const
            = default;
    };

    struct StoredEmployment final
    {
        std::vector<StoredJob> jobs;

        std::int32_t ticksUntilDispatch = 0;

        [[nodiscard]] bool operator==(
            const StoredEmployment &other) const = default;
    };

    [[nodiscard]] std::int32_t employedCount(const Employment &employment);

    void setEmployment(
        World &world,
        antwika::ecs::Entity entity,
        const Employment &employment);

}
