#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief How much of one resource a storehouse holds.
     *
     * Four times what anything else does, which is the whole reason a
     * storehouse is worth the three cells it stands on: a cart sets out
     * with kWalkerLoad, so a building capped at kStockCapacity is full
     * after one of them and every cart after that is turned away.
     */
    inline constexpr std::int32_t kStoreCapacity = 4 * kStockCapacity;

    /**
     * @brief Get how much of one resource a kind of building holds.
     *
     * A table rather than a comparison against Storage, for the reason
     * consumes() is one: the second kind that holds more than a house
     * would otherwise be a second name in the same expression.
     *
     * @param kind The kind to ask about.
     * @return Its capacity, per resource.
     */
    [[nodiscard]] constexpr std::int32_t capacityOf(
        BuildingKind kind) noexcept
    {
        constexpr std::array<std::int32_t, kBuildingKindCount> capacities{
            kStockCapacity,  // House
            kStockCapacity,  // Farm
            kStockCapacity,  // ClayPit
            kStockCapacity,  // Workshop
            kStoreCapacity,  // Storage
            kStockCapacity,  // Market
            kStockCapacity,  // Well
            kStockCapacity,  // Doctor
            kStockCapacity,  // FireStation
            kStockCapacity,  // EngineerPost
        };

        return capacities[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief Check whether a cart pusher may unload a resource here.
     *
     * **This is about where a cart is sent, not about what a building
     * may hold.** A house holds the food a market seller handed it and
     * is still not a place a farm's cart goes: goods reach a house
     * through a seller walking past it, which is the whole point of
     * there being a market at all.
     *
     * **A storehouse and nothing else, and the reason is the walk
     * phase rather than the fiction.** A cart's load changes hands in
     * BuildingSystem, which runs beside SpawnSystem in one phase --
     * so both read the building as of the last commit and both write
     * the whole component back, and the cadence's write is the later
     * of the two. A delivery into any kind that sends walkers is
     * therefore undone in the same tick it was made, silently, some of
     * the time. A storehouse is the one kind that sends nobody, which
     * is what makes it the one kind a cart can safely fill.
     *
     * That is not a workaround dressed up as a rule: it is why a market
     * is supplied by a buyer of its own, credited in a later phase
     * where nothing else writes it, rather than by a cart -- and it is
     * why nothing yet carts clay into a workshop, which would need a
     * walker kind this round's vocabulary has not got.
     *
     * @param kind The kind that would be unloaded into.
     * @param resource The resource in the cart.
     * @return True when a cart carrying it may be sent there.
     */
    [[nodiscard]] constexpr bool acceptsAt(
        BuildingKind kind, Resource resource) noexcept
    {
        constexpr std::array<
            std::array<bool, kResourceCount>, kBuildingKindCount> accepts{{
            // Food   Clay   Pottery
            {false, false, false},  // House
            {false, false, false},  // Farm
            {false, false, false},  // ClayPit
            {false, false, false},  // Workshop
            {true, true, true},     // Storage
            {false, false, false},  // Market
            {false, false, false},  // Well
            {false, false, false},  // Doctor
            {false, false, false},  // FireStation
            {false, false, false},  // EngineerPost
        }};

        return accepts[buildingKindIndex(kind) % kBuildingKindCount]
                      [resourceIndex(resource) % kResourceCount];
    }

    // A cart may only fill a kind that sends nobody.
    // Or the cadence undoes the delivery in the phase it was made.
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

    /**
     * @brief Check whether a market buyer may fetch from this kind.
     *
     * A storehouse and nothing else, and the asymmetry with acceptsAt()
     * is deliberate: a market that could buy from another market would
     * send the same load round in circles, and a market that could buy
     * from a farm would make the storehouse pointless.
     *
     * @param kind The kind to ask about.
     * @return True when a buyer may be sent there.
     */
    [[nodiscard]] constexpr bool suppliesMarkets(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> supplies{
            false,  // House
            false,  // Farm
            false,  // ClayPit
            false,  // Workshop
            true,   // Storage
            false,  // Market
            false,  // Well
            false,  // Doctor
            false,  // FireStation
            false,  // EngineerPost
        };

        return supplies[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief Get how much of one resource an entity is holding.
     *
     * **An entity with no Building answers zero rather than throwing**,
     * which is what makes this a query anything may ask about anything.
     * That is the shape every consumer of this header depends on: a
     * rule that reads a stock it may not find has an answer for the
     * ordinary case rather than an exception to catch.
     *
     * @param world The world to read, as of its last commit().
     * @param entity The entity to ask about.
     * @param resource The resource to count.
     * @return How much it holds, or zero when it holds nothing at all.
     */
    [[nodiscard]] std::int32_t stockOf(
        const antwika::ecs::World &world,
        antwika::ecs::Entity entity,
        Resource resource);

    // A storehouse takes everything, or it is not a storehouse.
    static_assert(acceptsAt(BuildingKind::Storage, Resource::Clay));
    static_assert(acceptsAt(BuildingKind::Storage, Resource::Food));
    static_assert(!acceptsAt(BuildingKind::Market, Resource::Food));
    static_assert(!acceptsAt(BuildingKind::House, Resource::Food));
    static_assert(capacityOf(BuildingKind::Storage) == kStoreCapacity);
    static_assert(capacityOf(BuildingKind::House) == kStockCapacity);
    static_assert(suppliesMarkets(BuildingKind::Storage));
    static_assert(!suppliesMarkets(BuildingKind::Market));

    // Somewhere a buyer may fetch from is somewhere a cart may go.
    // Otherwise a storehouse would empty and never fill again.
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

} // namespace antwika::game
