#include "antwika/holdem/HandCategory.hpp"

#include <string_view>

namespace antwika::holdem
{

    std::string_view toString(HandCategory category) noexcept
    {
        switch (category)
        {
            case HandCategory::HighCard:
                return "High Card";
            case HandCategory::OnePair:
                return "One Pair";
            case HandCategory::TwoPair:
                return "Two Pair";
            case HandCategory::ThreeOfAKind:
                return "Three of a Kind";
            case HandCategory::Straight:
                return "Straight";
            case HandCategory::Flush:
                return "Flush";
            case HandCategory::FullHouse:
                return "Full House";
            case HandCategory::FourOfAKind:
                return "Four of a Kind";
            case HandCategory::StraightFlush:
                return "Straight Flush";
        }

        return "Unknown";
    }

}
