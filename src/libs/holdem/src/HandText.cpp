#include "antwika/holdem/HandText.hpp"

#include <string>
#include <string_view>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandCategory.hpp"
#include "antwika/holdem/HandValue.hpp"

namespace antwika::holdem
{

    namespace
    {

        struct RankName
        {
            std::string_view one;
            std::string_view many;
        };

        [[nodiscard]] RankName nameOf(unsigned rank) noexcept
        {
            switch (static_cast<Rank>(rank))
            {
                case Rank::Two:
                    return {"Deuce", "Deuces"};
                case Rank::Three:
                    return {"Three", "Threes"};
                case Rank::Four:
                    return {"Four", "Fours"};
                case Rank::Five:
                    return {"Five", "Fives"};
                case Rank::Six:
                    return {"Six", "Sixes"};
                case Rank::Seven:
                    return {"Seven", "Sevens"};
                case Rank::Eight:
                    return {"Eight", "Eights"};
                case Rank::Nine:
                    return {"Nine", "Nines"};
                case Rank::Ten:
                    return {"Ten", "Tens"};
                case Rank::Jack:
                    return {"Jack", "Jacks"};
                case Rank::Queen:
                    return {"Queen", "Queens"};
                case Rank::King:
                    return {"King", "Kings"};
                case Rank::Ace:
                    return {"Ace", "Aces"};
            }

            return {"Unknown", "Unknowns"};
        }

        // Slots were filled most-significant first, below the category.
        // So slot zero is the rank the category leads with.
        [[nodiscard]] unsigned rankAt(
            HandValue value, unsigned slot) noexcept
        {
            const auto shift = kCategoryShift - (kSlotBits * (slot + 1U));
            const auto mask = (1U << kSlotBits) - 1U;
            return (rawValue(value) >> shift) & mask;
        }

        [[nodiscard]] std::string oneAt(HandValue value, unsigned slot)
        {
            return std::string(nameOf(rankAt(value, slot)).one);
        }

        [[nodiscard]] std::string manyAt(HandValue value, unsigned slot)
        {
            return std::string(nameOf(rankAt(value, slot)).many);
        }

        // A straight is stored as its top card alone.
        // The wheel is the one run not reaching four ranks below that.
        // Its ace plays under the five instead.
        [[nodiscard]] std::string runTo(HandValue value)
        {
            const auto top = rankAt(value, 0);
            const auto low = top == rawValue(Rank::Five)
                                 ? unsigned{rawValue(Rank::Ace)}
                                 : top - 4U;
            return std::string(nameOf(low).one) + " to "
                   + std::string(nameOf(top).one);
        }

    } // namespace

    std::string describe(HandValue value)
    {
        switch (categoryOf(value))
        {
            case HandCategory::HighCard:
                return "high card " + oneAt(value, 0);
            case HandCategory::OnePair:
                return "a pair of " + manyAt(value, 0);
            case HandCategory::TwoPair:
                return "two pair, " + manyAt(value, 0) + " and "
                       + manyAt(value, 1);
            case HandCategory::ThreeOfAKind:
                return "three of a kind, " + manyAt(value, 0);
            case HandCategory::Straight:
                return "a straight, " + runTo(value);
            case HandCategory::Flush:
                return "a flush, " + oneAt(value, 0) + " high";
            case HandCategory::FullHouse:
                return "a full house, " + manyAt(value, 0) + " full of "
                       + manyAt(value, 1);
            case HandCategory::FourOfAKind:
                return "four of a kind, " + manyAt(value, 0);
            case HandCategory::StraightFlush:
                return "a straight flush, " + runTo(value);
        }

        return "an unknown hand";
    }

} // namespace antwika::holdem
