#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    inline constexpr std::int32_t kProductionPeriodTicks = kTicksPerSecond;

    inline constexpr std::int32_t kProductionBatch = kStockCapacity / 4;

    struct Production final
    {
        std::int32_t ticksUntilOutput = kProductionPeriodTicks;

        [[nodiscard]] bool operator==(const Production &other) const
            = default;
    };

    [[nodiscard]] constexpr std::optional<Resource> producedBy(
        BuildingKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Resource>, kBuildingKindCount> produces{
            std::nullopt,
            Resource::Food,
            Resource::Clay,
            Resource::Pottery,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
        };

        return antwika::enums::pick(produces, kind);
    }

    [[nodiscard]] constexpr std::optional<Resource> consumedToProduce(
        BuildingKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Resource>, kBuildingKindCount> consumes{
            std::nullopt,
            std::nullopt,
            std::nullopt,
            Resource::Clay,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
        };

        return antwika::enums::pick(consumes, kind);
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if (consumedToProduce(kind).has_value()
                    && !producedBy(kind).has_value())
                {
                    return false;
                }
            }

            return true;
        }(),
        "a kind that consumes to produce must produce something");

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if (consumedToProduce(kind).has_value()
                    && consumedToProduce(kind) == producedBy(kind))
                {
                    return false;
                }
            }

            return true;
        }(),
        "a kind must not consume what it produces");

    static_assert(producedBy(BuildingKind::Farm) == Resource::Food);
    static_assert(!producedBy(BuildingKind::Storage).has_value());
    static_assert(
        consumedToProduce(BuildingKind::Workshop) == Resource::Clay);
    static_assert(!consumedToProduce(BuildingKind::Farm).has_value());

}
