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

    inline constexpr std::size_t kMaxStaffSources = 4;

    struct StaffEntry final
    {
        antwika::ecs::Entity house = antwika::ecs::kNullEntity;

        std::int32_t count = 0;

        [[nodiscard]] bool operator==(const StaffEntry &other) const
            = default;
    };

    struct Staff final
    {
        std::array<StaffEntry, kMaxStaffSources> sources{};

        std::int32_t ticksUntilDecay = 0;

        [[nodiscard]] bool operator==(const Staff &other) const = default;
    };

    inline constexpr std::int32_t kStaffDecayPeriodTicks = 500;

    struct StoredStaffEntry final
    {
        std::size_t house = 0;

        std::int32_t count = 0;

        [[nodiscard]] bool operator==(
            const StoredStaffEntry &other) const = default;
    };

    struct StoredStaff final
    {
        std::vector<StoredStaffEntry> entries;

        std::int32_t ticksUntilDecay = 0;

        [[nodiscard]] bool operator==(const StoredStaff &other) const
            = default;
    };

    [[nodiscard]] std::int32_t staffCount(const Staff &staff);

    void setStaff(
        World &world, antwika::ecs::Entity entity, const Staff &staff);

}
