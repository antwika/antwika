#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Interactions.hpp>

#include "antwika/poker/PokerAtlas.hpp"
#include "antwika/poker/SeatSnapshot.hpp"
#include "antwika/poker/TableScene.hpp"
#include "antwika/poker/TableSnapshot.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::holdem::parseCards;
using antwika::holdem::Stage;
using antwika::poker::SeatSnapshot;
using antwika::poker::TableScene;
using antwika::poker::TableSnapshot;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};

    constexpr Color kInk{.red = 232, .green = 236, .blue = 232};
    constexpr Color kDim{.red = 120, .green = 140, .blue = 128};
    constexpr Color kFelt{.red = 12, .green = 68, .blue = 44};
    constexpr Color kRedSuit{.red = 176, .green = 32, .blue = 32};
    constexpr Color kBlackSuit{.red = 24, .green = 24, .blue = 28};
    constexpr Color kToAct{.red = 232, .green = 196, .blue = 72};
    constexpr Color kInFront{.red = 224, .green = 176, .blue = 64};

    [[nodiscard]] SeatSnapshot player(
        std::string name, antwika::poker::Chips stack)
    {
        return SeatSnapshot{
            .name = std::move(name),
            .stack = stack,
            .occupied = true,
        };
    }

    [[nodiscard]] TableSnapshot idleTable()
    {
        return TableSnapshot{
            .tableName = "Antwika",
            .seats = {SeatSnapshot{}, SeatSnapshot{}},
            .blinds = {.small = 5, .big = 10},
        };
    }

    [[nodiscard]] TableSnapshot liveTable()
    {
        auto alice = player("alice", 380);
        alice.inHand = true;
        alice.isButton = true;
        alice.holeCards = {parseCards("As Kh")[0], parseCards("As Kh")[1]};

        auto bob = player("bob", 120);
        bob.inHand = true;
        bob.isToAct = true;
        bob.roundCommitted = 40;
        bob.holeCards = {parseCards("7d 7c")[0], parseCards("7d 7c")[1]};

        return TableSnapshot{
            .tableName = "Antwika",
            .seats = {alice, bob},
            .board = parseCards("Ah Kd 7s"),
            .pot = 240,
            .blinds = {.small = 5, .big = 10},
            .stage = Stage::Flop,
            .handsPlayed = 12,
            .handInProgress = true,
        };
    }
} // namespace

// Each test pins the one thing it is about.
// Everything else the scene draws is allowed and ignored.
// So a layout tweak does not break every case in this file.
class TableSceneTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        EXPECT_CALL(renderer, clear(_)).Times(AnyNumber());
        EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
        EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(renderer, present()).Times(AnyNumber());
    }

    NiceMock<MockRenderer> renderer;
    TableScene scene;
};

TEST_F(TableSceneTest, Draw_ClearsTheFeltFirst)
{
    EXPECT_CALL(renderer, clear(kFelt));

    scene.draw(renderer, kCanvas, idleTable());
}

TEST_F(TableSceneTest, Draw_HeadsAnIdleTableWithWhatItIsWaitingFor)
{
    EXPECT_CALL(
        renderer, drawText(_, "Antwika -- waiting for players", 2, kInk));

    scene.draw(renderer, kCanvas, idleTable());
}

TEST_F(TableSceneTest, Draw_HeadsALiveTableWithTheHandAndTheStage)
{
    EXPECT_CALL(renderer, drawText(_, "Antwika -- hand 12 -- flop", 2, kInk));

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_ShowsTheBlindsAndThePot)
{
    EXPECT_CALL(renderer, drawText(_, "blinds 5/10", 2, kDim));
    EXPECT_CALL(renderer, drawText(_, "pot 240", 2, kInk));

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_ColoursTheBoardBySuit)
{
    // Ah and Kd are red, 7s is black.
    EXPECT_CALL(renderer, drawText(_, "Ah", 2, kRedSuit));
    EXPECT_CALL(renderer, drawText(_, "Kd", 2, kRedSuit));
    EXPECT_CALL(renderer, drawText(_, "7s", 2, kBlackSuit));

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_DrawsNoBoardBeforeTheFlop)
{
    auto snapshot = liveTable();
    snapshot.board.clear();

    EXPECT_CALL(renderer, drawText(_, "Ah", _, _)).Times(0);
    EXPECT_CALL(renderer, drawText(_, "pot 240", 2, kInk));

    scene.draw(renderer, kCanvas, snapshot);
}

TEST_F(TableSceneTest, Draw_LabelsAnEmptySeatAsEmpty)
{
    EXPECT_CALL(renderer, drawText(_, "-- empty --", 2, kDim)).Times(2);

    scene.draw(renderer, kCanvas, idleTable());
}

TEST_F(TableSceneTest, Draw_NamesEverySeatedPlayerAndTheirStack)
{
    EXPECT_CALL(renderer, drawText(_, "alice (D)", 2, kInk));
    EXPECT_CALL(renderer, drawText(_, "380", 2, kInk));
    EXPECT_CALL(renderer, drawText(_, "bob", 2, kInk));
    EXPECT_CALL(renderer, drawText(_, "120", 2, kInk));

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_WithholdsTheButtonBadgeUntilTheFirstDeal)
{
    auto snapshot = liveTable();
    snapshot.handsPlayed = 0;

    // Before a hand is dealt the button is wherever Table put it.
    EXPECT_CALL(renderer, drawText(_, "alice (D)", _, _)).Times(0);
    EXPECT_CALL(renderer, drawText(_, "alice", 2, kInk));

    scene.draw(renderer, kCanvas, snapshot);
}

TEST_F(TableSceneTest, Draw_DimsASeatThatIsNotInTheHand)
{
    auto snapshot = liveTable();
    snapshot.seats[1].inHand = false;

    EXPECT_CALL(renderer, drawText(_, "bob", 2, kDim));

    scene.draw(renderer, kCanvas, snapshot);
}

TEST_F(TableSceneTest, Draw_ShowsTheHoleCardsOfEverySeatInTheHand)
{
    EXPECT_CALL(renderer, drawText(_, "As", 2, kBlackSuit));
    EXPECT_CALL(renderer, drawText(_, "Kh", 2, kRedSuit));
    EXPECT_CALL(renderer, drawText(_, "7d", 2, kRedSuit));
    EXPECT_CALL(renderer, drawText(_, "7c", 2, kBlackSuit));

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_DrawsNoHoleCardsForASeatOutOfTheHand)
{
    auto snapshot = liveTable();
    snapshot.seats[1].inHand = false;

    EXPECT_CALL(renderer, drawText(_, "7d", _, _)).Times(0);
    EXPECT_CALL(renderer, drawText(_, "7c", _, _)).Times(0);

    scene.draw(renderer, kCanvas, snapshot);
}

TEST_F(TableSceneTest, Draw_ShowsWhatASeatHasBetThisRound)
{
    EXPECT_CALL(renderer, drawText(_, "bet 40", 2, kInFront));

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_ShowsNoBetForASeatThatHasNotActed)
{
    auto snapshot = liveTable();
    snapshot.seats[1].roundCommitted = 0;

    EXPECT_CALL(renderer, drawText(_, "bet 40", _, _)).Times(0);

    scene.draw(renderer, kCanvas, snapshot);
}

TEST_F(TableSceneTest, Draw_RingsTheSeatThatHasToAct)
{
    // Exactly one seat is waiting, so exactly one gets the ring.
    EXPECT_CALL(renderer, drawRect(_, kToAct)).Times(1);

    scene.draw(renderer, kCanvas, liveTable());
}

TEST_F(TableSceneTest, Draw_RingsNobodyBetweenHands)
{
    EXPECT_CALL(renderer, drawRect(_, kToAct)).Times(0);

    scene.draw(renderer, kCanvas, idleTable());
}

TEST_F(TableSceneTest, Draw_ScalesTheTextToTheCanvas)
{
    // 720 rows is three glyph pixels per pixel, 640 was two.
    EXPECT_CALL(renderer, drawText(_, "pot 240", 3, kInk));

    scene.draw(renderer, Size{.width = 1280, .height = 720}, liveTable());
}

TEST_F(TableSceneTest, Draw_KeepsTheSmallestCanvasAtOnePixelPerGlyphPixel)
{
    EXPECT_CALL(renderer, drawText(_, "pot 240", 1, kInk));

    scene.draw(renderer, Size{.width = 320, .height = 200}, liveTable());
}

TEST_F(TableSceneTest, Draw_KeepsANineSeatTableInsideTheCanvas)
{
    TableSnapshot snapshot{
        .tableName = "Antwika",
        .blinds = {.small = 5, .big = 10},
        .handsPlayed = 1,
    };
    for (int index = 0; index < 9; ++index)
    {
        snapshot.seats.push_back(player("p" + std::to_string(index), 100));
    }

    // A short window is where rows would otherwise run off the bottom.
    const Size canvas{.width = 640, .height = 480};
    std::vector<Rect> rects;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawRect(_, _))
        .Times(AnyNumber())
        .WillRepeatedly([&rects](Rect rect, Color) { rects.push_back(rect); });

    scene.draw(renderer, canvas, snapshot);

    for (const auto &rect : rects)
    {
        const auto bottom =
            rect.origin.y + static_cast<std::int32_t>(rect.size.height);
        EXPECT_LE(bottom, static_cast<std::int32_t>(canvas.height));
    }
}

TEST_F(TableSceneTest, Draw_DrawsNoSeatsForATableWithNone)
{
    TableSnapshot snapshot{
        .tableName = "Antwika",
        .blinds = {.small = 5, .big = 10},
    };

    EXPECT_CALL(renderer, drawText(_, "-- empty --", _, _)).Times(0);

    scene.draw(renderer, kCanvas, snapshot);
}

TEST_F(TableSceneTest, Draw_ShrinksRowsThatCannotAllFitRatherThanOverflowing)
{
    // Nine rows want more height than this canvas has at all.
    // The layout shares out what there is instead of running past it.
    TableSnapshot snapshot{
        .tableName = "Antwika",
        .blinds = {.small = 5, .big = 10},
        .handsPlayed = 1,
    };
    for (int index = 0; index < 9; ++index)
    {
        snapshot.seats.push_back(player("p" + std::to_string(index), 100));
    }

    constexpr Size canvas{.width = 320, .height = 200};
    std::vector<Rect> rects;
    EXPECT_CALL(renderer, drawRect(_, _))
        .Times(AnyNumber())
        .WillRepeatedly([&rects](Rect rect, Color) { rects.push_back(rect); });

    scene.draw(renderer, canvas, snapshot);

    ASSERT_FALSE(rects.empty());
    for (const auto &rect : rects)
    {
        EXPECT_GE(rect.origin.y, 0);
        EXPECT_LE(
            rect.origin.y + static_cast<std::int32_t>(rect.size.height),
            static_cast<std::int32_t>(canvas.height));
    }
}

TEST_F(TableSceneTest, Describe_IsThePictureDrawPaints)
{
    // The frame is the picture as a value.
    // So one table compares against another with no renderer involved.
    const auto frame = scene.describe(kCanvas, liveTable());

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_EQ(frame.commands, scene.describe(kCanvas, liveTable()).commands);

    // Nothing on this table can be pointed at, so nothing reports being.
    EXPECT_EQ(frame.interactions, antwika::ui::Interactions{});
}

TEST_F(TableSceneTest, Describe_FillsTheRailAcrossTheTopOfTheCanvas)
{
    constexpr Color kRail{.red = 40, .green = 26, .blue = 18};

    const auto frame = scene.describe(kCanvas, idleTable());

    ASSERT_FALSE(frame.commands.empty());
    const auto *rail = std::get_if<antwika::ui::FillRect>(&frame.commands[0]);

    ASSERT_NE(rail, nullptr);
    EXPECT_EQ(rail->color, kRail);
    EXPECT_EQ(rail->rect.origin.x, 0);
    EXPECT_EQ(rail->rect.origin.y, 0);
    EXPECT_EQ(rail->rect.size.width, kCanvas.width);
}

// --- The art layer -------------------------------------------------

namespace
{
    using antwika::poker::ArtBlit;

    [[nodiscard]] std::size_t blitsOf(
        const std::vector<ArtBlit> &art, Rect source)
    {
        std::size_t found = 0;
        for (const auto &blit : art)
        {
            found += blit.source == source ? 1U : 0U;
        }

        return found;
    }

    [[nodiscard]] bool anyBlitTinted(
        const std::vector<ArtBlit> &art, Rect source, Color tint)
    {
        for (const auto &blit : art)
        {
            if (blit.source == source && blit.tint == tint)
            {
                return true;
            }
        }

        return false;
    }
} // namespace

TEST(TableArtTest, DescribeArt_TilesTheFeltAcrossTheWholeCanvas)
{
    const TableScene scene;
    const auto art = scene.describeArt(kCanvas, idleTable());

    const auto felt = antwika::poker::sourceOf(antwika::poker::kFeltSlot);
    const auto across = kCanvas.width / antwika::poker::kAtlasSlotSize.width;
    const auto down = kCanvas.height / antwika::poker::kAtlasSlotSize.height;

    EXPECT_EQ(blitsOf(art, felt), across * down);

    // The first tile is the top-left corner, exactly one slot across.
    ASSERT_FALSE(art.empty());
    EXPECT_EQ(
        art.front(),
        (ArtBlit{
            .source = felt,
            .destination =
                Rect{
                    .origin = {.x = 0, .y = 0},
                    .size = antwika::poker::kAtlasSlotSize},
            .tint = Color{
                .red = 255, .green = 255, .blue = 255, .alpha = 255}}));
}

TEST(TableArtTest, DescribeArt_GivesEverySeatAPlate)
{
    const TableScene scene;
    auto snapshot = idleTable();
    const auto art = scene.describeArt(kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kPlateSlot)),
        snapshot.seats.size());
}

TEST(TableArtTest, DescribeArt_SeatsOnlyTheOccupied)
{
    const TableScene scene;
    auto snapshot = liveTable();
    snapshot.seats.push_back(SeatSnapshot{});

    const auto art = scene.describeArt(kCanvas, snapshot);

    // A chair each for those taken, a plate each for all of them.
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kChairSlot)),
        snapshot.seats.size() - 1);
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kPlateSlot)),
        snapshot.seats.size());
}

TEST(TableArtTest, DescribeArt_DrawsTheBoardFaceUpAsThreeBlitsACard)
{
    const TableScene scene;
    auto snapshot = idleTable();
    snapshot.board = parseCards("Ah Kd 7c");

    const auto art = scene.describeArt(kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kCardFaceSlot)),
        3U);
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kCardBackSlot)),
        0U);

    // A red suit and a black one are the same glyph, tinted apart.
    EXPECT_TRUE(anyBlitTinted(
        art,
        antwika::poker::sourceOfSuit(antwika::holdem::Suit::Hearts),
        kRedSuit));
    EXPECT_TRUE(anyBlitTinted(
        art,
        antwika::poker::sourceOfSuit(antwika::holdem::Suit::Clubs),
        kBlackSuit));
}

TEST(TableArtTest, DescribeArt_KeepsHoleCardsFaceDownBeforeShowdown)
{
    const TableScene scene;
    auto snapshot = liveTable();
    snapshot.board.clear();
    snapshot.stage = Stage::Flop;

    const auto art = scene.describeArt(kCanvas, snapshot);

    // Two seats in the hand, two cards each, none of them anybody's.
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kCardBackSlot)),
        4U);
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kCardFaceSlot)),
        0U);
}

TEST(TableArtTest, DescribeArt_TurnsHoleCardsOverAtShowdown)
{
    const TableScene scene;
    auto snapshot = liveTable();
    snapshot.board.clear();
    snapshot.stage = Stage::Showdown;

    const auto art = scene.describeArt(kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kCardBackSlot)),
        0U);
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kCardFaceSlot)),
        4U);
}

TEST(TableArtTest, DescribeArt_MarksTheButtonTheBetAndWhoseTurnItIs)
{
    const TableScene scene;
    const auto snapshot = liveTable();

    const auto art = scene.describeArt(kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(
            art, antwika::poker::sourceOf(antwika::poker::kDealerButtonSlot)),
        1U);
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kToActSlot)),
        1U);
    // One for the pot, one in front of the seat that has bet.
    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kChipSlot)),
        2U);
}

TEST(TableArtTest, DescribeArt_HidesTheButtonBeforeTheFirstDeal)
{
    const TableScene scene;
    auto snapshot = liveTable();
    snapshot.handsPlayed = 0;

    const auto art = scene.describeArt(kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(
            art, antwika::poker::sourceOf(antwika::poker::kDealerButtonSlot)),
        0U);
}

TEST(TableArtTest, DescribeArt_SurvivesACanvasWithNoRoomAboveTheSeats)
{
    const TableScene scene;
    auto snapshot = idleTable();
    snapshot.board = parseCards("Ah Kd 7c");

    // Too short for the seat rows, let alone a board above them.
    const auto art =
        scene.describeArt(Size{.width = 40, .height = 24}, snapshot);

    EXPECT_FALSE(art.empty());
    for (const auto &blit : art)
    {
        EXPECT_GE(blit.destination.origin.x, 0);
        EXPECT_GE(blit.destination.origin.y, 0);
    }
}

// A card is 8 pixels wide at the floor, so a full board is 44 across
// and does not fit a canvas 40 wide.
// The centring subtracts, and these are unsigned: without the guard the
// board would start at four billion and disappear entirely.
TEST(TableArtTest, DescribeArt_LeftAlignsABoardWiderThanTheCanvas)
{
    const TableScene scene;
    auto snapshot = idleTable();
    snapshot.board = parseCards("Ah Kd 7c 2s 9h");

    const auto art =
        scene.describeArt(Size{.width = 40, .height = 400}, snapshot);

    const auto face =
        antwika::poker::sourceOf(antwika::poker::kCardFaceSlot);
    ASSERT_EQ(blitsOf(art, face), 5U);
    for (const auto &blit : art)
    {
        if (blit.source == face)
        {
            EXPECT_GE(blit.destination.origin.x, 0);
            EXPECT_LT(blit.destination.origin.x, 44);
        }
    }
}

// A picture two runs disagree about is two different tables, so every
// field has to count -- and each one is asserted on its own, since a
// defaulted operator== stops at the first difference it finds.
TEST(TableArtTest, EveryFieldOfABlitCountsTowardsEquality)
{
    const ArtBlit base{
        .source = antwika::poker::sourceOf(antwika::poker::kFeltSlot),
        .destination =
            Rect{
                .origin = {.x = 4, .y = 8},
                .size = {.width = 32, .height = 32}},
        .tint = Color{
            .red = 255, .green = 255, .blue = 255, .alpha = 255}};

    EXPECT_EQ(base, base);

    ArtBlit other = base;
    other.source =
        antwika::poker::sourceOf(antwika::poker::kCardBackSlot);
    EXPECT_NE(base, other);

    other = base;
    other.destination.origin.x = 5;
    EXPECT_NE(base, other);

    other = base;
    other.tint = kRedSuit;
    EXPECT_NE(base, other);
}

TEST(TableArtTest, DescribeArt_DrawsNothingForASeatlessTable)
{
    const TableScene scene;
    TableSnapshot snapshot;

    const auto art = scene.describeArt(kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kPlateSlot)),
        0U);
}

TEST(TableArtTest, Draw_PaintsTheArtBeforeTheText)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<antwika::gfx::mocks::MockTexture> atlas;
    const TableScene scene;
    const auto snapshot = idleTable();

    EXPECT_CALL(renderer, clear(_)).Times(1);
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(scene.describeArt(kCanvas, snapshot).size()));

    scene.draw(renderer, kCanvas, snapshot, &atlas);
}

TEST(TableArtTest, Draw_DrawsNoTextureWithoutAnAtlas)
{
    NiceMock<MockRenderer> renderer;
    const TableScene scene;

    EXPECT_CALL(renderer, clear(_)).Times(1);
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    scene.draw(renderer, kCanvas, idleTable());
}
