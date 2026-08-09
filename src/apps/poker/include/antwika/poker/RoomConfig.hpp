#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/Chips.hpp>

#include "antwika/poker/AgentStyle.hpp"

namespace antwika::poker
{

    using antwika::holdem::Blinds;
    using antwika::holdem::kHandCategoryCount;
    using antwika::holdem::Chips;

    inline constexpr std::array<unsigned, kHandCategoryCount>
        kDefaultHandStrengths{20, 45, 62, 76, 85, 90, 95, 98, 100};

    struct AgentThresholds final
    {
        unsigned raiseAt = 0;

        unsigned callAt = 0;

        [[nodiscard]] bool operator==(
            const AgentThresholds &other) const = default;
    };

    inline constexpr std::size_t kSeatStyleCount = 3;

    inline constexpr std::array<AgentStyle, kSeatStyleCount>
        kDefaultSeatStyles{
            AgentStyle::Balanced,
            AgentStyle::Tight,
            AgentStyle::Aggressive};

    inline constexpr std::array<AgentThresholds, kAgentStyleCount>
        kDefaultThresholds{
            AgentThresholds{.raiseAt = 80, .callAt = 55},
            AgentThresholds{.raiseAt = 70, .callAt = 40},
            AgentThresholds{.raiseAt = 55, .callAt = 25}};

    struct RoomConfig final
    {
        std::size_t seatCount = 6;

        Blinds blinds{.small = 5, .big = 10};

        Chips minimumBuyIn = 100;

        std::string tableName = "Antwika";

        std::uint64_t seed = 1;

        std::array<AgentThresholds, kAgentStyleCount> thresholds =
            kDefaultThresholds;

        std::array<AgentStyle, kSeatStyleCount> seatStyles =
            kDefaultSeatStyles;

        std::array<unsigned, kHandCategoryCount> handStrengths =
            kDefaultHandStrengths;

        bool operator==(const RoomConfig &other) const = default;
    };

}
