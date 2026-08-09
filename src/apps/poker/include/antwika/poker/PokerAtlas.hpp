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

    inline constexpr Size kAtlasSlotSize{.width = 32, .height = 32};

    inline constexpr std::uint32_t kAtlasColumns = 8;

    inline constexpr std::uint32_t kAtlasRows = 4;

    inline constexpr Size kAtlasSize{
        .width = kAtlasColumns * kAtlasSlotSize.width,
        .height = kAtlasRows * kAtlasSlotSize.height};

    inline constexpr std::uint32_t kCardFaceSlot = 0;

    inline constexpr std::uint32_t kCardBackSlot = 1;

    inline constexpr std::uint32_t kFeltSlot = 2;

    inline constexpr std::uint32_t kPlateSlot = 3;

    inline constexpr std::uint32_t kChairSlot = 4;

    inline constexpr std::uint32_t kChipSlot = 5;

    inline constexpr std::uint32_t kDealerButtonSlot = 6;

    inline constexpr std::uint32_t kToActSlot = 7;

    inline constexpr std::uint32_t kTableSlot = 12;

    inline constexpr std::uint32_t kFirstSuitSlot = 8;

    inline constexpr std::uint32_t kFirstRankSlot = 16;

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

    [[nodiscard]] constexpr Rect sourceOfSuit(Suit suit) noexcept
    {
        return sourceOf(kFirstSuitSlot + rawValue(suit));
    }

    [[nodiscard]] constexpr Rect sourceOfRank(Rank rank) noexcept
    {
        return sourceOf(kFirstRankSlot + rawValue(rank));
    }

    [[nodiscard]] constexpr bool isRedSuit(Card card) noexcept
    {
        const auto suit = suitOf(card);

        return suit == Suit::Hearts || suit == Suit::Diamonds;
    }

    [[nodiscard]] constexpr Rect rankSourceOf(Card card) noexcept
    {
        return sourceOfRank(rankOf(card));
    }

    [[nodiscard]] constexpr Rect suitSourceOf(Card card) noexcept
    {
        return sourceOfSuit(suitOf(card));
    }

}
