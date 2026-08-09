#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    inline constexpr std::int32_t kBurnDurationTicks =
        20 * kTicksPerSecond;

    inline constexpr std::int32_t kSpreadDelayTicks =
        10 * kTicksPerSecond;

    enum class RuinState : std::uint8_t
    {
        Burning = 0,
        Debris,
    };

    [[nodiscard]] constexpr RuinState enumBound(RuinState) noexcept
    {
        return RuinState::Debris;
    }

    inline constexpr std::size_t kRuinStateCount =
        antwika::enums::kCount<RuinState>;

    [[nodiscard]] constexpr std::size_t ruinStateIndex(
        const RuinState state) noexcept
    {
        return antwika::enums::index(state);
    }

    [[nodiscard]] constexpr std::string_view ruinStateName(
        RuinState state) noexcept
    {
        constexpr std::array<std::string_view, kRuinStateCount> names{
            "burning", "debris"};

        return antwika::enums::pick(names, state);
    }

    [[nodiscard]] constexpr std::optional<RuinState> ruinStateFromName(
        std::string_view name) noexcept
    {
        for (std::size_t index = 0; index < kRuinStateCount; ++index)
        {
            const auto state = static_cast<RuinState>(index);

            if (ruinStateName(state) == name)
            {
                return state;
            }
        }

        return std::nullopt;
    }

    static_assert(ruinStateName(RuinState::Burning) == "burning");
    static_assert(ruinStateFromName("debris") == RuinState::Debris);
    static_assert(!ruinStateFromName("ashes").has_value());

    struct Ruin final
    {
        BuildingKind kind = BuildingKind::House;

        RuinState state = RuinState::Burning;

        std::int32_t ticksUntilOut = kBurnDurationTicks;

        [[nodiscard]] bool operator==(const Ruin &other) const = default;
    };

}
