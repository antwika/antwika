#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/NameTable.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Store.hpp"

namespace antwika::game
{

    enum class HousingLevel : std::uint8_t
    {
        Tent = 0,
        Shack,
        Hovel,
        Cottage,
    };

    [[nodiscard]] constexpr HousingLevel enumBound(HousingLevel) noexcept
    {
        return HousingLevel::Cottage;
    }

    inline constexpr std::size_t kHousingLevelCount =
        antwika::enums::kCount<HousingLevel>;

    [[nodiscard]] constexpr std::size_t housingLevelIndex(
        const HousingLevel level) noexcept
    {
        return antwika::enums::index(level);
    }

    inline constexpr auto kHousingLevels =
        antwika::enums::kAll<HousingLevel>;

    inline constexpr antwika::enums::NameTable<HousingLevel>
        kHousingLevelNames{{"tent", "shack", "hovel", "cottage"}};

    [[nodiscard]] constexpr std::string_view housingLevelName(
        const HousingLevel level) noexcept
    {
        return kHousingLevelNames.name(level);
    }

    [[nodiscard]] constexpr std::optional<HousingLevel>
        housingLevelFromName(const std::string_view name) noexcept
    {
        return kHousingLevelNames.from(name);
    }

    [[nodiscard]] constexpr bool housesPeople(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> housing{
            true,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
        };

        return antwika::enums::pick(housing, kind);
    }

    struct HousingRequirement final
    {
        std::int32_t desirability = 0;

        std::array<bool, kServiceCount> services{};

        std::array<std::int32_t, kResourceCount> goods{};

        std::int32_t populationCapacity = 0;

        [[nodiscard]] constexpr bool operator==(
            const HousingRequirement &other) const = default;
    };

    inline constexpr std::array<HousingRequirement, kHousingLevelCount>
        kHousingRequirements{{
            {.desirability = 0,
             .services = {},
             .goods = {},
             .populationCapacity = 5},

            {.desirability = 0,
             .services = {true, false},
             .goods = {},
             .populationCapacity = 10},

            {.desirability = 1,
             .services = {true, false},
             .goods = {25, 0, 0},
             .populationCapacity = 16},

            {.desirability = 2,
             .services = {true, true},
             .goods = {50, 0, 20},
             .populationCapacity = 24},
        }};

    [[nodiscard]] constexpr HousingRequirement requirementOf(
        HousingLevel level) noexcept
    {
        return antwika::enums::pick(kHousingRequirements, level);
    }

    [[nodiscard]] constexpr std::int32_t stockCapacityOf(
        HousingLevel level) noexcept
    {
        return kStockCapacity
            * (static_cast<std::int32_t>(
                   housingLevelIndex(level) % kHousingLevelCount)
               + 1);
    }

    inline constexpr std::int32_t kEvolvePeriodTicks =
        4 * kTicksPerSecond;

    inline constexpr std::int32_t kDevolvePeriodTicks =
        4 * kTicksPerSecond;

    static_assert(
        kHousingRequirements[0]
            == HousingRequirement{
                .desirability = 0,
                .services = {},
                .goods = {},
                .populationCapacity =
                    kHousingRequirements[0].populationCapacity},
        "the bottom level must demand nothing");

    static_assert(
        []
        {
            for (std::size_t index = 1; index < kHousingLevelCount; ++index)
            {
                const auto &below = kHousingRequirements[index - 1];
                const auto &here = kHousingRequirements[index];

                if (here.desirability < below.desirability
                    || here.populationCapacity < below.populationCapacity)
                {
                    return false;
                }

                for (std::size_t slot = 0; slot < kServiceCount; ++slot)
                {
                    if (below.services[slot] && !here.services[slot])
                    {
                        return false;
                    }
                }

                for (std::size_t slot = 0; slot < kResourceCount; ++slot)
                {
                    if (here.goods[slot] < below.goods[slot])
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "every housing demand must rise with the level");

    static_assert(
        []
        {
            for (const auto &requirement : kHousingRequirements)
            {
                for (const auto resource : kResources)
                {
                    if (requirement.goods[resourceIndex(resource)] > 0
                        && !fetchesFromStores(
                            BuildingKind::Market, resource))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a level may only demand a good a market brings home");

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kHousingLevelCount;
                 ++index)
            {
                const auto &requirement = kHousingRequirements[index];
                const auto held =
                    stockCapacityOf(static_cast<HousingLevel>(index));

                for (const auto resource : kResources)
                {
                    if (requirement.goods[resourceIndex(resource)]
                        > held)
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a level may only demand what a house on it can hold");

    static_assert(
        []
        {
            for (std::size_t index = 1; index < kHousingLevelCount;
                 ++index)
            {
                if (stockCapacityOf(static_cast<HousingLevel>(index))
                    <= stockCapacityOf(
                        static_cast<HousingLevel>(index - 1)))
                {
                    return false;
                }
            }

            return true;
        }(),
        "stock capacity must rise with the level");

    static_assert(
        stockCapacityOf(HousingLevel::Tent) == kStockCapacity);

    static_assert(housingLevelName(HousingLevel::Tent) == "tent");
    static_assert(
        housingLevelFromName("cottage") == HousingLevel::Cottage);
    static_assert(!housingLevelFromName("villa").has_value());
    static_assert(housesPeople(BuildingKind::House));
    static_assert(!housesPeople(BuildingKind::Market));

}
