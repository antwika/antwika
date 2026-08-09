#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::holdem
{

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

    [[nodiscard]] constexpr HandCategory enumBound(HandCategory) noexcept
    {
        return HandCategory::StraightFlush;
    }

    inline constexpr std::size_t kHandCategoryCount =
        antwika::enums::kCount<HandCategory>;

    [[nodiscard]] std::string_view toString(HandCategory category) noexcept;

}
