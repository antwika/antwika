#pragma once

#include <cstdint>
#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

namespace antwika::game
{

    using antwika::ecs::World;

    struct Staffing final
    {
        std::int32_t filled = 0;

        std::int32_t wanted = 0;

        [[nodiscard]] constexpr bool operator==(
            const Staffing &other) const = default;
    };

    [[nodiscard]] Staffing staffingOf(
        const World &world, antwika::ecs::Entity entity);

    [[nodiscard]] std::int32_t workersAt(
        const World &world, antwika::ecs::Entity entity);

    [[nodiscard]] constexpr std::optional<std::int32_t> workedPeriod(
        std::int32_t period, Staffing staffing) noexcept
    {
        if (staffing.wanted <= 0 || staffing.filled >= staffing.wanted)
        {
            return period;
        }

        if (staffing.filled <= 0)
        {
            return std::nullopt;
        }

        return period * staffing.wanted / staffing.filled;
    }

    static_assert(
        workedPeriod(10, Staffing{.filled = 0, .wanted = 0}) == 10);
    static_assert(
        workedPeriod(10, Staffing{.filled = 4, .wanted = 4}) == 10);
    static_assert(
        workedPeriod(10, Staffing{.filled = 2, .wanted = 4}) == 20);
    static_assert(
        !workedPeriod(10, Staffing{.filled = 0, .wanted = 4}).has_value());

}
