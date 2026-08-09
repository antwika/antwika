#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    [[nodiscard]] constexpr std::int32_t workersWantedBy(
        BuildingKind kind) noexcept
    {
        constexpr std::array<std::int32_t, kBuildingKindCount> wanted{
            0,
            4,
            4,
            4,
            0,
            3,
            1,
            2,
            2,
            2,
        };

        return antwika::enums::pick(wanted, kind);
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if ((workersWantedBy(kind) > 0) != sendsWalkers(kind))
                {
                    return false;
                }
            }

            return true;
        }(),
        "a kind wants workers exactly when it sends somebody out");

    static_assert(workersWantedBy(BuildingKind::House) == 0);
    static_assert(workersWantedBy(BuildingKind::Storage) == 0);
    static_assert(workersWantedBy(BuildingKind::Well) > 0);

}
