#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::animation
{

    /**
     * @brief One of the four ways a directional clip set can face.
     *
     * The names and their order match antwika::game::Direction, so an
     * app can cast one to the other, but this library deliberately does
     * not include that header: a facing here is just a way of choosing
     * between four clips, and nothing about it is a grid.
     *
     * Values are contiguous from zero, so a facing can index a table.
     */
    enum class Facing : std::uint8_t
    {
        North = 0,
        East,
        South,
        West,
    };

    /**
     * @brief How many facings there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kFacingCount =
        static_cast<std::size_t>(Facing::West) + 1;

    /**
     * @brief Get a facing's index, for addressing a per-facing table.
     * @param facing The facing to index.
     * @return The index, always below kFacingCount for a named facing.
     */
    [[nodiscard]] constexpr std::size_t facingIndex(Facing facing) noexcept
    {
        return static_cast<std::size_t>(facing);
    }

} // namespace antwika::animation
