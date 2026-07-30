#pragma once

#include <cstdint>

namespace antwika::holdem
{

    /**
     * @brief A card's rank, ordered from weakest to strongest.
     *
     * The enumerator values are the rank's bit position in the 13-bit
     * rank masks HandEvaluator works with, which is why Two is 0 and
     * Ace is 12 rather than the printed face values.
     */
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

    /**
     * @brief A card's suit.
     *
     * Suits carry no strength of their own -- they only separate cards
     * that share a rank, and pick flushes out of a hand.
     */
    enum class Suit : std::uint8_t
    {
        Clubs = 0,
        Diamonds,
        Hearts,
        Spades,
    };

    /**
     * @brief One of the 52 cards, packed as `(rank << 2) | suit`.
     *
     * A scoped enum with no enumerators, the same idiom as
     * antwika::scheduler::JobId: it behaves like the byte it is, but
     * cannot be confused with an unrelated integer. The packing is what
     * lets a card double as a bit position in a 52-bit deck mask and be
     * split back into rank and suit with a shift and a mask, with no
     * lookup table anywhere.
     */
    enum class Card : std::uint8_t
    {
    };

    /**
     * @brief Number of distinct ranks in a deck.
     */
    inline constexpr std::uint8_t kRankCount = 13;

    /**
     * @brief Number of distinct suits in a deck.
     */
    inline constexpr std::uint8_t kSuitCount = 4;

    /**
     * @brief Number of cards in a full deck.
     */
    inline constexpr std::uint8_t kCardCount = kRankCount * kSuitCount;

    /**
     * @brief Bits a Card reserves for its suit, below its rank.
     */
    inline constexpr std::uint8_t kSuitBits = 2;

    /**
     * @brief Get the raw integer value backing a card.
     * @param card The card to unwrap.
     * @return The packed `(rank << 2) | suit` byte, in [0, 52).
     */
    [[nodiscard]] constexpr std::uint8_t rawValue(Card card) noexcept
    {
        return static_cast<std::uint8_t>(card);
    }

    /**
     * @brief Get the bit position a rank occupies in a rank mask.
     * @param rank The rank to unwrap.
     * @return The underlying value, in [0, 13).
     */
    [[nodiscard]] constexpr std::uint8_t rawValue(Rank rank) noexcept
    {
        return static_cast<std::uint8_t>(rank);
    }

    /**
     * @brief Get the index a suit occupies in a per-suit array.
     * @param suit The suit to unwrap.
     * @return The underlying value, in [0, 4).
     */
    [[nodiscard]] constexpr std::uint8_t rawValue(Suit suit) noexcept
    {
        return static_cast<std::uint8_t>(suit);
    }

    /**
     * @brief Build a card from its rank and suit.
     * @param rank The card's rank.
     * @param suit The card's suit.
     * @return The packed card.
     */
    [[nodiscard]] constexpr Card makeCard(Rank rank, Suit suit) noexcept
    {
        return static_cast<Card>(
            static_cast<std::uint8_t>(rawValue(rank) << kSuitBits)
            | rawValue(suit));
    }

    /**
     * @brief Extract a card's rank.
     * @param card The card to inspect.
     * @return The rank held in the card's upper bits.
     */
    [[nodiscard]] constexpr Rank rankOf(Card card) noexcept
    {
        return static_cast<Rank>(rawValue(card) >> kSuitBits);
    }

    /**
     * @brief Extract a card's suit.
     * @param card The card to inspect.
     * @return The suit held in the card's lower bits.
     */
    [[nodiscard]] constexpr Suit suitOf(Card card) noexcept
    {
        return static_cast<Suit>(
            rawValue(card) & ((1U << kSuitBits) - 1U));
    }

    /**
     * @brief Get the single-bit rank mask a rank corresponds to.
     * @param rank The rank to convert.
     * @return A 13-bit mask with only this rank's bit set.
     */
    [[nodiscard]] constexpr std::uint16_t rankBit(Rank rank) noexcept
    {
        return static_cast<std::uint16_t>(1U << rawValue(rank));
    }

} // namespace antwika::holdem
