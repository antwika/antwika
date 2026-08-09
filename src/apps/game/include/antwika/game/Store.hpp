#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    inline constexpr std::int32_t kStoreCapacity = 4 * kStockCapacity;

    [[nodiscard]] constexpr std::int32_t capacityOf(
        BuildingKind kind) noexcept
    {
        constexpr std::array<std::int32_t, kBuildingKindCount> capacities{
            kStockCapacity,
            kStockCapacity,
            kStockCapacity,
            kStockCapacity,
            kStoreCapacity,
            kStockCapacity,
            kStockCapacity,
            kStockCapacity,
            kStockCapacity,
            kStockCapacity,
        };

        return antwika::enums::pick(capacities, kind);
    }

    [[nodiscard]] constexpr bool acceptsAt(
        BuildingKind kind, Resource resource) noexcept
    {
        constexpr std::array<
            std::array<bool, kResourceCount>, kBuildingKindCount> accepts{{
            {false, false, false},
            {false, false, false},
            {false, false, false},
            {false, false, false},
            {true, true, true},
            {false, false, false},
            {false, false, false},
            {false, false, false},
            {false, false, false},
            {false, false, false},
        }};

        return accepts[buildingKindIndex(kind) % kBuildingKindCount]
                      [resourceIndex(resource) % kResourceCount];
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                for (const auto resource : kResources)
                {
                    if (acceptsAt(kind, resource) && sendsWalkers(kind))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a cart may only unload into a kind that sends nobody");

    [[nodiscard]] constexpr bool suppliesMarkets(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> supplies{
            false,
            false,
            false,
            false,
            true,
            false,
            false,
            false,
            false,
            false,
        };

        return antwika::enums::pick(supplies, kind);
    }

    [[nodiscard]] constexpr bool fetchesFromStores(
        BuildingKind kind, Resource resource) noexcept
    {
        constexpr std::array<
            std::array<bool, kResourceCount>, kBuildingKindCount> fetches{{
            {false, false, false},
            {false, false, false},
            {false, false, false},
            {false, true, false},
            {false, false, false},
            {true, false, true},
            {false, false, false},
            {false, false, false},
            {false, false, false},
            {false, false, false},
        }};

        return fetches[buildingKindIndex(kind) % kBuildingKindCount]
                      [resourceIndex(resource) % kResourceCount];
    }

    [[nodiscard]] constexpr bool fetchesFromStores(
        BuildingKind kind) noexcept
    {
        for (const auto resource : kResources)
        {
            if (fetchesFromStores(kind, resource))
            {
                return true;
            }
        }

        return false;
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if (consumedToProduce(kind).has_value()
                    && !fetchesFromStores(kind, *consumedToProduce(kind)))
                {
                    return false;
                }
            }

            return true;
        }(),
        "a kind that consumes to produce must fetch what it consumes");

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                for (const auto resource : kResources)
                {
                    if (fetchesFromStores(kind, resource)
                        && !acceptsAt(BuildingKind::Storage, resource))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a fetched resource must be one a store may be filled with");

    static_assert(
        fetchesFromStores(BuildingKind::Workshop, Resource::Clay));
    static_assert(
        !fetchesFromStores(BuildingKind::Workshop, Resource::Pottery));
    static_assert(fetchesFromStores(BuildingKind::Market, Resource::Food));
    static_assert(
        fetchesFromStores(BuildingKind::Market, Resource::Pottery));
    static_assert(!fetchesFromStores(BuildingKind::Farm));

    [[nodiscard]] std::int32_t stockOf(
        const antwika::ecs::World &world,
        antwika::ecs::Entity entity,
        Resource resource);

    static_assert(acceptsAt(BuildingKind::Storage, Resource::Clay));
    static_assert(acceptsAt(BuildingKind::Storage, Resource::Food));
    static_assert(!acceptsAt(BuildingKind::Market, Resource::Food));
    static_assert(!acceptsAt(BuildingKind::House, Resource::Food));
    static_assert(capacityOf(BuildingKind::Storage) == kStoreCapacity);
    static_assert(capacityOf(BuildingKind::House) == kStockCapacity);
    static_assert(suppliesMarkets(BuildingKind::Storage));
    static_assert(!suppliesMarkets(BuildingKind::Market));

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if (!suppliesMarkets(kind))
                {
                    continue;
                }

                for (const auto resource : kResources)
                {
                    if (!acceptsAt(kind, resource))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a kind a buyer fetches from must be one a cart may fill");

}
