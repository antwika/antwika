#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::holdem
{

    /**
     * @brief The nine kinds of made poker hand, ordered from weakest to
     * strongest.
     *
     * The enumerator values are the high-order field of a HandValue, so
     * comparing two categories numerically is the same comparison as
     * comparing the hands they came from -- see HandValue.
     */
    enum class HandCategory : std::uint8_t
    {
        HighCard = 0,
        OnePair,
        TwoPair,
        ThreeOfAKind,
        Straight,
        Flush,
        FullHouse,
        FourOfAKind,
        StraightFlush,
    };

    /**
     * @brief Render a category as the name a player would use for it.
     * @param category The category to name.
     * @return A human-readable name, e.g. "Full House".
     */
    [[nodiscard]] std::string_view toString(HandCategory category) noexcept;

} // namespace antwika::holdem
