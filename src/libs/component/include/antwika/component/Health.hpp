#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::component
{

    inline constexpr std::uint16_t kFullHealth = 240;

    inline constexpr std::uint16_t kMealWorth = kFullHealth / 2;

    inline constexpr std::uint16_t kHungryAt = kFullHealth / 4;

    inline constexpr time::Tick kHungerTicks = 96;

    inline constexpr time::Tick kThirstTicks = 64;

    struct Health final
    {
        std::uint16_t food = kFullHealth;

        std::uint16_t water = kFullHealth;

        [[nodiscard]] bool operator==(
            const Health &other) const = default;
    };

}
