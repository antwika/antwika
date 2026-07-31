#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <utility>

#include <antwika/holdem/Card.hpp>

#include "antwika/poker/PokerAtlas.hpp"

using antwika::gfx::Rect;
using antwika::holdem::Card;
using antwika::holdem::kRankCount;
using antwika::holdem::kSuitCount;
using antwika::holdem::makeCard;
using antwika::holdem::Rank;
using antwika::holdem::Suit;
using antwika::poker::isRedSuit;
using antwika::poker::kAtlasColumns;
using antwika::poker::kAtlasRows;
using antwika::poker::kAtlasSize;
using antwika::poker::kAtlasSlotSize;
using antwika::poker::kFirstRankSlot;
using antwika::poker::kFirstSuitSlot;
using antwika::poker::rankSourceOf;
using antwika::poker::sourceOf;
using antwika::poker::sourceOfRank;
using antwika::poker::sourceOfSuit;
using antwika::poker::suitSourceOf;

TEST(PokerAtlasTest, SourceOf_PutsTheFirstSlotAtTheOrigin)
{
    EXPECT_EQ(
        sourceOf(0),
        (Rect{.origin = {.x = 0, .y = 0}, .size = kAtlasSlotSize}));
}

TEST(PokerAtlasTest, SourceOf_RunsLeftToRightThenDown)
{
    EXPECT_EQ(sourceOf(1).origin.x, 32);
    EXPECT_EQ(sourceOf(1).origin.y, 0);

    EXPECT_EQ(sourceOf(kAtlasColumns).origin.x, 0);
    EXPECT_EQ(sourceOf(kAtlasColumns).origin.y, 32);
}

TEST(PokerAtlasTest, SourceOf_KeepsEverySlotInsideTheAtlas)
{
    for (std::uint32_t slot = 0; slot < kAtlasColumns * kAtlasRows; ++slot)
    {
        const auto source = sourceOf(slot);

        EXPECT_LE(
            static_cast<std::uint32_t>(source.origin.x) + source.size.width,
            kAtlasSize.width);
        EXPECT_LE(
            static_cast<std::uint32_t>(source.origin.y) + source.size.height,
            kAtlasSize.height);
    }
}

TEST(PokerAtlasTest, SourceOfSuit_GivesEverySuitASlotOfItsOwn)
{
    std::set<std::pair<std::int32_t, std::int32_t>> seen;

    for (std::uint8_t raw = 0; raw < kSuitCount; ++raw)
    {
        const auto source = sourceOfSuit(static_cast<Suit>(raw));
        seen.emplace(source.origin.x, source.origin.y);
    }

    EXPECT_EQ(seen.size(), kSuitCount);
    EXPECT_EQ(sourceOfSuit(Suit::Clubs), sourceOf(kFirstSuitSlot));
}

TEST(PokerAtlasTest, SourceOfRank_GivesEveryRankASlotOfItsOwn)
{
    std::set<std::pair<std::int32_t, std::int32_t>> seen;

    for (std::uint8_t raw = 0; raw < kRankCount; ++raw)
    {
        const auto source = sourceOfRank(static_cast<Rank>(raw));
        seen.emplace(source.origin.x, source.origin.y);
    }

    EXPECT_EQ(seen.size(), kRankCount);
    EXPECT_EQ(sourceOfRank(Rank::Two), sourceOf(kFirstRankSlot));
    EXPECT_EQ(
        sourceOfRank(Rank::Ace), sourceOf(kFirstRankSlot + kRankCount - 1));
}

TEST(PokerAtlasTest, RankSourceOf_SplitsACardIntoItsTwoGlyphs)
{
    const auto card = makeCard(Rank::Queen, Suit::Hearts);

    EXPECT_EQ(rankSourceOf(card), sourceOfRank(Rank::Queen));
    EXPECT_EQ(suitSourceOf(card), sourceOfSuit(Suit::Hearts));
}

TEST(PokerAtlasTest, IsRedSuit_IsTrueForHeartsAndDiamondsOnly)
{
    EXPECT_TRUE(isRedSuit(makeCard(Rank::Ace, Suit::Hearts)));
    EXPECT_TRUE(isRedSuit(makeCard(Rank::Ace, Suit::Diamonds)));
    EXPECT_FALSE(isRedSuit(makeCard(Rank::Ace, Suit::Clubs)));
    EXPECT_FALSE(isRedSuit(makeCard(Rank::Ace, Suit::Spades)));
}

TEST(PokerAtlasTest, Atlas_IsAsBigAsItsSlotGridSaysItIs)
{
    EXPECT_EQ(kAtlasSize.width, kAtlasColumns * kAtlasSlotSize.width);
    EXPECT_EQ(kAtlasSize.height, kAtlasRows * kAtlasSlotSize.height);
}
