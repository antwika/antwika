#include "antwika/holdem/HandEvaluator.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <optional>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandCategory.hpp"
#include "antwika/holdem/HandEvaluationError.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Limits.hpp"

namespace antwika::holdem
{

    namespace
    {

        struct Slots final
        {
            std::uint32_t packed = 0;
            unsigned used = 0;
        };

        [[nodiscard]] unsigned highestRank(std::uint16_t mask) noexcept
        {
            return static_cast<unsigned>(std::bit_width(mask)) - 1U;
        }

        [[nodiscard]] std::uint16_t withoutRank(
            std::uint16_t mask, unsigned rank) noexcept
        {
            return static_cast<std::uint16_t>(mask & ~(1U << rank));
        }

        void appendRank(Slots &slots, unsigned rank) noexcept
        {
            slots.packed = (slots.packed << kSlotBits) | rank;
            ++slots.used;
        }

        void appendTopRanks(
            Slots &slots, std::uint16_t mask, unsigned count) noexcept
        {
            for (unsigned taken = 0; taken < count; ++taken)
            {
                const auto rank = highestRank(mask);
                appendRank(slots, rank);
                mask = withoutRank(mask, rank);
            }
        }

        [[nodiscard]] HandValue compose(
            HandCategory category, const Slots &slots) noexcept
        {
            const auto shift = kSlotBits * (kSlotCount - slots.used);
            return static_cast<HandValue>(
                (static_cast<std::uint32_t>(category) << kCategoryShift)
                | (slots.packed << shift));
        }

        [[nodiscard]] std::optional<unsigned> straightTopRank(
            std::uint16_t rankMask) noexcept
        {
            const std::uint32_t aceLow =
                (rankMask >> rawValue(Rank::Ace)) & 1U;
            const std::uint32_t extended =
                (static_cast<std::uint32_t>(rankMask) << 1U) | aceLow;
            const std::uint32_t runs = extended & (extended >> 1U)
                                       & (extended >> 2U)
                                       & (extended >> 3U)
                                       & (extended >> 4U);
            if (runs == 0)
            {
                return std::nullopt;
            }

            const auto lowestOfBestRun =
                static_cast<unsigned>(std::bit_width(runs)) - 1U;
            return lowestOfBestRun + 3U;
        }

        [[nodiscard]] std::array<std::uint16_t, kSuitCount> rankMasksBySuit(
            std::span<const Card> cards)
        {
            std::array<std::uint16_t, kSuitCount> bySuit{};
            std::uint64_t seen = 0;
            for (const auto card : cards)
            {
                const auto index = rawValue(card);
                if (index >= kCardCount)
                {
                    throw HandEvaluationError(
                        "HandEvaluator: card is not one of the 52");
                }

                const auto bit = std::uint64_t{1} << index;
                if ((seen & bit) != 0)
                {
                    throw HandEvaluationError(
                        "HandEvaluator: the same card appears twice");
                }
                seen |= bit;

                auto &suitMask = bySuit[rawValue(suitOf(card))];
                suitMask = static_cast<std::uint16_t>(
                    suitMask | rankBit(rankOf(card)));
            }
            return bySuit;
        }

    }

    HandValue evaluate(std::span<const Card> cards)
    {
        if (cards.size() < kMinEvaluatedCards
            || cards.size() > kMaxEvaluatedCards)
        {
            throw HandEvaluationError(
                "HandEvaluator: a hand holds 5 to 7 cards");
        }

        const auto bySuit = rankMasksBySuit(cards);
        const auto clubs = bySuit[rawValue(Suit::Clubs)];
        const auto diamonds = bySuit[rawValue(Suit::Diamonds)];
        const auto hearts = bySuit[rawValue(Suit::Hearts)];
        const auto spades = bySuit[rawValue(Suit::Spades)];

        const auto anyRank = static_cast<std::uint16_t>(
            clubs | diamonds | hearts | spades);
        const auto twiceOver = static_cast<std::uint16_t>(
            (clubs & diamonds) | (clubs & hearts) | (clubs & spades)
            | (diamonds & hearts) | (diamonds & spades)
            | (hearts & spades));
        const auto thriceOver = static_cast<std::uint16_t>(
            (clubs & diamonds & hearts) | (clubs & diamonds & spades)
            | (clubs & hearts & spades) | (diamonds & hearts & spades));
        const auto quads = static_cast<std::uint16_t>(
            clubs & diamonds & hearts & spades);
        const auto trips = static_cast<std::uint16_t>(
            thriceOver & ~quads);
        const auto pairs = static_cast<std::uint16_t>(
            twiceOver & ~thriceOver);

        std::uint16_t flush = 0;
        for (const auto suitMask : bySuit)
        {
            if (static_cast<std::size_t>(std::popcount(suitMask))
                >= kHandSize)
            {
                flush = suitMask;
            }
        }

        if (flush != 0)
        {
            if (const auto top = straightTopRank(flush))
            {
                Slots slots;
                appendRank(slots, *top);
                return compose(HandCategory::StraightFlush, slots);
            }
        }

        if (quads != 0)
        {
            const auto quadRank = highestRank(quads);
            Slots slots;
            appendRank(slots, quadRank);
            appendTopRanks(slots, withoutRank(anyRank, quadRank), 1);
            return compose(HandCategory::FourOfAKind, slots);
        }

        if (trips != 0)
        {
            const auto tripsRank = highestRank(trips);
            const auto fillers = static_cast<std::uint16_t>(
                pairs | withoutRank(trips, tripsRank));
            if (fillers != 0)
            {
                Slots slots;
                appendRank(slots, tripsRank);
                appendRank(slots, highestRank(fillers));
                return compose(HandCategory::FullHouse, slots);
            }
        }

        if (flush != 0)
        {
            Slots slots;
            appendTopRanks(slots, flush, kHandSize);
            return compose(HandCategory::Flush, slots);
        }

        if (const auto top = straightTopRank(anyRank))
        {
            Slots slots;
            appendRank(slots, *top);
            return compose(HandCategory::Straight, slots);
        }

        if (trips != 0)
        {
            const auto tripsRank = highestRank(trips);
            Slots slots;
            appendRank(slots, tripsRank);
            appendTopRanks(slots, withoutRank(anyRank, tripsRank), 2);
            return compose(HandCategory::ThreeOfAKind, slots);
        }

        if (std::popcount(pairs) >= 2)
        {
            const auto highPair = highestRank(pairs);
            const auto lowPair = highestRank(withoutRank(pairs, highPair));
            Slots slots;
            appendRank(slots, highPair);
            appendRank(slots, lowPair);
            appendTopRanks(
                slots,
                withoutRank(withoutRank(anyRank, highPair), lowPair),
                1);
            return compose(HandCategory::TwoPair, slots);
        }

        if (pairs != 0)
        {
            const auto pairRank = highestRank(pairs);
            Slots slots;
            appendRank(slots, pairRank);
            appendTopRanks(slots, withoutRank(anyRank, pairRank), 3);
            return compose(HandCategory::OnePair, slots);
        }

        Slots slots;
        appendTopRanks(slots, anyRank, kHandSize);
        return compose(HandCategory::HighCard, slots);
    }

}
