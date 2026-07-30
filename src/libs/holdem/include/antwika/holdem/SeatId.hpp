#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::holdem
{

    /**
     * @brief Identifies one seat at a table, counted clockwise from
     * zero.
     *
     * A seat outlives the player sitting in it: position relative to the
     * button is what the betting rules are written in terms of, so the
     * table deals in seats and leaves the question of who occupies one
     * to its caller.
     */
    enum class SeatId : std::uint8_t
    {
    };

    /**
     * @brief Get the raw integer value backing a seat id.
     * @param seat The seat id to unwrap.
     * @return The underlying std::uint8_t.
     */
    [[nodiscard]] constexpr std::uint8_t rawValue(SeatId seat) noexcept
    {
        return static_cast<std::uint8_t>(seat);
    }

    /**
     * @brief Get a seat id as an array index.
     * @param seat The seat id to unwrap.
     * @return The underlying value, widened for indexing.
     */
    [[nodiscard]] constexpr std::size_t indexOf(SeatId seat) noexcept
    {
        return static_cast<std::size_t>(rawValue(seat));
    }

    /**
     * @brief Build a seat id from a seat index.
     * @param index The seat's clockwise position, in [0, kMaxSeats).
     * @return The corresponding seat id.
     */
    [[nodiscard]] constexpr SeatId makeSeatId(std::size_t index) noexcept
    {
        return static_cast<SeatId>(static_cast<std::uint8_t>(index));
    }

} // namespace antwika::holdem
