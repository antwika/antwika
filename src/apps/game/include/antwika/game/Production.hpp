#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief Ticks between one batch of output and the next.
     *
     * One a second, derived from kTicksPerSecond exactly as every other
     * period is, so changing the pace stays one edit in Building.hpp.
     */
    inline constexpr std::int32_t kProductionPeriodTicks = kTicksPerSecond;

    /**
     * @brief How much one batch is, in and out.
     *
     * One number rather than two, because a workshop that turned clay
     * into pottery at anything but parity would be a conversion rate --
     * a balance decision this increment has no evidence for and would
     * have to persist to stay honest across a rename.
     * A quarter of kStockCapacity, so a producer left alone fills up
     * in four batches and a cart is worth sending after one.
     */
    inline constexpr std::int32_t kProductionBatch = kStockCapacity / 4;

    /**
     * @brief What a building is part-way through making.
     *
     * One countdown, per building, in the building's own component --
     * for the reason Building's three are: two workshops put up a tick
     * apart would otherwise finish in lockstep for ever, and a replay
     * regenerates each countdown from the same click that placed it.
     *
     * **Absent means a fresh countdown rather than a broken building.**
     * ProductionSystem gives one to any producer that has none, so a
     * building placed now and a building read out of a file written
     * before this component existed both start a batch from the top.
     */
    struct Production
    {
        /** @brief Ticks until the next batch is finished. */
        std::int32_t ticksUntilOutput = kProductionPeriodTicks;

        /**
         * @brief Compare two production states.
         * @param other The state to compare against.
         * @return True when both countdowns match.
         */
        [[nodiscard]] bool operator==(const Production &other) const
            = default;
    };

    /**
     * @brief Get what a kind of building makes, if it makes anything.
     *
     * A table for footprintOf()'s reason: the ghost, the save and the
     * system all have to agree about a kind before any entity of it
     * exists, which a component cannot tell them.
     *
     * @param kind The kind to ask about.
     * @return The resource it produces, or nullopt for one that
     * produces nothing.
     */
    [[nodiscard]] constexpr std::optional<Resource> producedBy(
        BuildingKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Resource>, kBuildingKindCount> produces{
            std::nullopt,        // House
            Resource::Food,      // Farm
            Resource::Clay,      // ClayPit
            Resource::Pottery,   // Workshop
            std::nullopt,        // Storage
            std::nullopt,        // Market
            std::nullopt,        // Well
            std::nullopt,        // Doctor
            std::nullopt,        // FireStation
            std::nullopt,        // EngineerPost
        };

        return produces[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief Get what a kind of building has to consume to make its
     * output.
     *
     * @param kind The kind to ask about.
     * @return The resource one batch costs, or nullopt for a kind that
     * makes its output out of the ground.
     */
    [[nodiscard]] constexpr std::optional<Resource> consumedToProduce(
        BuildingKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Resource>, kBuildingKindCount> consumes{
            std::nullopt,     // House
            std::nullopt,     // Farm
            std::nullopt,     // ClayPit
            Resource::Clay,   // Workshop
            std::nullopt,     // Storage
            std::nullopt,     // Market
            std::nullopt,     // Well
            std::nullopt,     // Doctor
            std::nullopt,     // FireStation
            std::nullopt,     // EngineerPost
        };

        return consumes[buildingKindIndex(kind) % kBuildingKindCount];
    }

    // A kind that eats something to work has to make something.
    // Otherwise it is a kind that destroys goods on a countdown.
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

    // And it must not eat what it makes, or a batch is free.
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

} // namespace antwika::game
