#pragma once

#include <cstdint>

#include "antwika/ecs_commons/GridPosition.hpp"

namespace antwika::ecs_commons
{

    /**
     * @brief How far an entity moves each tick, in whole cells.
     *
     * Whole cells per tick rather than a fraction, for the reason
     * GridPosition gives: a sub-cell velocity would need a scale factor,
     * and a scale factor is either floating point or a fixed-point unit
     * this library would have to impose on every app that uses it.
     * An app wanting to move slower than one cell a tick composes
     * PeriodicSystem around MovementSystem instead, which keeps the rate
     * in integers too.
     */
    struct Velocity
    {
        std::int32_t dx = 0;
        std::int32_t dy = 0;

        /**
         * @brief Compare two velocities.
         * @param other The velocity to compare against.
         * @return True when both components match.
         */
        [[nodiscard]] bool operator==(const Velocity &other) const = default;
    };

    /**
     * @brief Apply a velocity to a position once.
     * @param position The position to move from.
     * @param velocity The per-tick step to add.
     * @return The position one step along.
     *
     * The addition is done in 64 bits and narrowed back, so a step that
     * would overflow wraps predictably rather than being undefined -- the
     * same result on every platform, which is what a replay needs.
     */
    [[nodiscard]] constexpr GridPosition stepBy(
        GridPosition position, Velocity velocity) noexcept
    {
        const auto x =
            static_cast<std::int64_t>(position.x) + velocity.dx;
        const auto y =
            static_cast<std::int64_t>(position.y) + velocity.dy;

        return GridPosition{
            .x = static_cast<std::int32_t>(static_cast<std::uint32_t>(x)),
            .y = static_cast<std::int32_t>(static_cast<std::uint32_t>(y))};
    }

} // namespace antwika::ecs_commons
