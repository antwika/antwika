#pragma once

#include <cstdint>

#include "antwika/holdem/HandCategory.hpp"

namespace antwika::holdem
{

    /**
     * @brief The strength of a made poker hand, as one comparable
     * integer.
     *
     * The contract is deliberately total: a greater value is a stronger
     * hand, and two equal values are hands that split a pot -- no
     * secondary tie-break exists, because every detail poker cares
     * about is already inside the number. The layout is
     * `category << 20` above five 4-bit rank slots, filled
     * most-significant first with the ranks that matter for that
     * category (the trips rank then the pair rank for a full house, the
     * pair rank then three kickers for one pair, and so on), and zero
     * in the slots the category does not use.
     *
     * A scoped enum rather than a bare integer so a hand's strength
     * cannot be mistaken for a chip count or a rank; relational
     * operators work on it directly.
     */
    enum class HandValue : std::uint32_t
    {
    };

    /**
     * @brief Bits one rank slot of a HandValue occupies.
     */
    inline constexpr unsigned kSlotBits = 4;

    /**
     * @brief Rank slots a HandValue carries below its category.
     */
    inline constexpr unsigned kSlotCount = 5;

    /**
     * @brief Bit position of a HandValue's category field.
     */
    inline constexpr unsigned kCategoryShift = kSlotBits * kSlotCount;

    /**
     * @brief Get the raw integer value backing a hand value.
     * @param value The hand value to unwrap.
     * @return The underlying std::uint32_t.
     */
    [[nodiscard]] constexpr std::uint32_t rawValue(HandValue value) noexcept
    {
        return static_cast<std::uint32_t>(value);
    }

    /**
     * @brief Recover which kind of hand a value describes.
     * @param value The hand value to inspect.
     * @return The category held in the value's high-order field.
     */
    [[nodiscard]] constexpr HandCategory categoryOf(HandValue value) noexcept
    {
        return static_cast<HandCategory>(rawValue(value) >> kCategoryShift);
    }

} // namespace antwika::holdem
