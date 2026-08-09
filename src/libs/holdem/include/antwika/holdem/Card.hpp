#pragma once

#include <cstdint>

namespace antwika::holdem
{

    enum class Rank : std::uint8_t
    {
        Two = 0,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,
        Ten,
        Jack,
        Queen,
        King,
        Ace,
    };

    enum class Suit : std::uint8_t
    {
        Clubs = 0,
        Diamonds,
        Hearts,
        Spades,
    };

    enum class Card : std::uint8_t
    {
    };

    inline constexpr std::uint8_t kRankCount = 13;

    inline constexpr std::uint8_t kSuitCount = 4;

    inline constexpr std::uint8_t kCardCount = kRankCount * kSuitCount;

    inline constexpr std::uint8_t kSuitBits = 2;

    [[nodiscard]] constexpr std::uint8_t rawValue(Card card) noexcept
    {
        return static_cast<std::uint8_t>(card);
    }

    [[nodiscard]] constexpr std::uint8_t rawValue(Rank rank) noexcept
    {
        return static_cast<std::uint8_t>(rank);
    }

    [[nodiscard]] constexpr std::uint8_t rawValue(Suit suit) noexcept
    {
        return static_cast<std::uint8_t>(suit);
    }

    [[nodiscard]] constexpr Card makeCard(Rank rank, Suit suit) noexcept
    {
        return static_cast<Card>(
            static_cast<std::uint8_t>(rawValue(rank) << kSuitBits)
            | rawValue(suit));
    }

    [[nodiscard]] constexpr Rank rankOf(Card card) noexcept
    {
        return static_cast<Rank>(rawValue(card) >> kSuitBits);
    }

    [[nodiscard]] constexpr Suit suitOf(Card card) noexcept
    {
        return static_cast<Suit>(
            rawValue(card) & ((1U << kSuitBits) - 1U));
    }

    [[nodiscard]] constexpr std::uint16_t rankBit(Rank rank) noexcept
    {
        return static_cast<std::uint16_t>(1U << rawValue(rank));
    }

}
