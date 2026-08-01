#pragma once

#include <cstdint>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Card.hpp>

namespace antwika::poker
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::holdem::Card;
    using antwika::holdem::Rank;
    using antwika::holdem::rankOf;
    using antwika::holdem::rawValue;
    using antwika::holdem::Suit;
    using antwika::holdem::suitOf;

    /**
     * @brief The size of one slot's cell in the atlas, in pixels.
     *
     * Square, because nothing in this atlas is a projection of anything:
     * a card, a chip and a glyph are all blitted into whatever rectangle
     * the layout gives them.
     */
    inline constexpr Size kAtlasSlotSize{.width = 32, .height = 32};

    /**
     * @brief How many slots a row of the atlas holds.
     */
    inline constexpr std::uint32_t kAtlasColumns = 8;

    /**
     * @brief How many rows of slots the atlas holds.
     */
    inline constexpr std::uint32_t kAtlasRows = 4;

    /**
     * @brief The size the atlas image must be, in pixels.
     */
    inline constexpr Size kAtlasSize{
        .width = kAtlasColumns * kAtlasSlotSize.width,
        .height = kAtlasRows * kAtlasSlotSize.height};

    /**
     * @brief The slot holding a blank card face.
     *
     * A rank and a suit glyph are blitted over it, which is what keeps
     * the atlas at seventeen glyphs instead of fifty-two faces.
     */
    inline constexpr std::uint32_t kCardFaceSlot = 0;

    /**
     * @brief The slot holding the back of a card.
     */
    inline constexpr std::uint32_t kCardBackSlot = 1;

    /**
     * @brief The slot holding a tile of table felt.
     */
    inline constexpr std::uint32_t kFeltSlot = 2;

    /**
     * @brief The slot holding the plate a player's details sit on.
     */
    inline constexpr std::uint32_t kPlateSlot = 3;

    /**
     * @brief The slot holding the chair behind a seat.
     */
    inline constexpr std::uint32_t kChairSlot = 4;

    /**
     * @brief The slot holding a stack of chips.
     */
    inline constexpr std::uint32_t kChipSlot = 5;

    /**
     * @brief The slot holding the dealer button.
     */
    inline constexpr std::uint32_t kDealerButtonSlot = 6;

    /**
     * @brief The slot holding the marker for whoever is acting.
     */
    inline constexpr std::uint32_t kToActSlot = 7;

    /**
     * @brief The slot holding the table itself.
     *
     * A rounded rectangle of felt inside a rail, drawn once and blitted
     * into whatever rectangle the layout gave the middle of the table.
     * antwika::gfx has no rounded-rectangle call and no clipping, so the
     * rounding is art rather than arithmetic: one blit says what a
     * stack of fills per corner would have, and it scales with the
     * window because a slot is stretched into its destination.
     */
    inline constexpr std::uint32_t kTableSlot = 12;

    /**
     * @brief The slot holding the first suit glyph, clubs.
     *
     * The four are in Suit order, so a suit's slot is its own value and
     * the two cannot drift apart.
     */
    inline constexpr std::uint32_t kFirstSuitSlot = 8;

    /**
     * @brief The slot holding the first rank glyph, the deuce.
     *
     * The thirteen are in Rank order, for the same reason the suits are.
     */
    inline constexpr std::uint32_t kFirstRankSlot = 16;

    /**
     * @brief Where a slot sits in the atlas.
     *
     * Slots run left to right and then down, so this is the one place
     * that turns a slot number into pixels.
     *
     * @param slot Which slot to locate.
     * @return The rectangle to blit from.
     */
    [[nodiscard]] constexpr Rect sourceOf(std::uint32_t slot) noexcept
    {
        return Rect{
            .origin =
                Point{
                    .x = static_cast<std::int32_t>(
                        (slot % kAtlasColumns) * kAtlasSlotSize.width),
                    .y = static_cast<std::int32_t>(
                        (slot / kAtlasColumns) * kAtlasSlotSize.height)},
            .size = kAtlasSlotSize};
    }

    /**
     * @brief Where a suit's glyph sits in the atlas.
     * @param suit The suit to draw.
     * @return The rectangle to blit from.
     */
    [[nodiscard]] constexpr Rect sourceOfSuit(Suit suit) noexcept
    {
        return sourceOf(kFirstSuitSlot + rawValue(suit));
    }

    /**
     * @brief Where a rank's glyph sits in the atlas.
     * @param rank The rank to draw.
     * @return The rectangle to blit from.
     */
    [[nodiscard]] constexpr Rect sourceOfRank(Rank rank) noexcept
    {
        return sourceOf(kFirstRankSlot + rawValue(rank));
    }

    /**
     * @brief Whether a card's suit is drawn in red.
     *
     * The suit glyphs are drawn white so that one blit each serves both
     * colours, which leaves the colour to the tint and so to this.
     *
     * @param card The card being drawn.
     * @return True for hearts and diamonds.
     */
    [[nodiscard]] constexpr bool isRedSuit(Card card) noexcept
    {
        const auto suit = suitOf(card);

        return suit == Suit::Hearts || suit == Suit::Diamonds;
    }

    /**
     * @brief Where a card's rank glyph sits in the atlas.
     * @param card The card being drawn.
     * @return The rectangle to blit from.
     */
    [[nodiscard]] constexpr Rect rankSourceOf(Card card) noexcept
    {
        return sourceOfRank(rankOf(card));
    }

    /**
     * @brief Where a card's suit glyph sits in the atlas.
     * @param card The card being drawn.
     * @return The rectangle to blit from.
     */
    [[nodiscard]] constexpr Rect suitSourceOf(Card card) noexcept
    {
        return sourceOfSuit(suitOf(card));
    }

} // namespace antwika::poker
