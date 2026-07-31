#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/WidgetRects.hpp>

#include "antwika/poker/PokerAtlas.hpp"
#include "antwika/poker/SeatSnapshot.hpp"
#include "antwika/poker/TableScene.hpp"
#include "antwika/poker/TableSnapshot.hpp"
#include "antwika/poker/TableWidgets.hpp"

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

    // The art is placed from the layout.
    // So a test asking for it describes the frame first.
    // Which is what draw() does.
    [[nodiscard]] std::vector<ArtBlit> artOf(
        const TableScene &scene, Size canvas,
        const TableSnapshot &snapshot)
    {
        return scene.describeArt(
            canvas, scene.describe(canvas, snapshot).rects, snapshot);
    }

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
    const auto art = artOf(scene, kCanvas, idleTable());

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
    const auto art = artOf(scene, kCanvas, snapshot);

    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kPlateSlot)),
        snapshot.seats.size());
}

// The art plates one row per seat and the ui boxes one row per seat.
// They are the same rows, so they have to be the same distance apart.
// One function answers how tall a row is, and this is why.
TEST(TableArtTest, DescribeArt_PlatesTheRowsTheUiBoxesAtTheSamePitch)
{
    const TableScene scene;
    auto snapshot = liveTable();
    snapshot.seats.push_back(SeatSnapshot{});
    ASSERT_GE(snapshot.seats.size(), 3U);

    const auto plate =
        antwika::poker::sourceOf(antwika::poker::kPlateSlot);

    std::vector<std::int32_t> plateTops;
    for (const auto &blit : artOf(scene, kCanvas, snapshot))
    {
        if (blit.source == plate)
        {
            plateTops.push_back(blit.destination.origin.y);
        }
    }

    // A seat's box is the only fill in that colour, one per seat.
    constexpr Color kSeatBox{.red = 16, .green = 50, .blue = 36};
    std::vector<std::int32_t> boxTops;
    for (const auto &command : scene.describe(kCanvas, snapshot).commands)
    {
        const auto *fill = std::get_if<antwika::ui::FillRect>(&command);
        if (fill != nullptr && fill->color == kSeatBox)
        {
            boxTops.push_back(fill->rect.origin.y);
        }
    }

    ASSERT_EQ(plateTops.size(), snapshot.seats.size());
    ASSERT_EQ(boxTops.size(), snapshot.seats.size());

    for (std::size_t index = 1; index < plateTops.size(); ++index)
    {
        EXPECT_EQ(
            plateTops[index] - plateTops[index - 1],
            boxTops[index] - boxTops[index - 1]);
    }
}

TEST(TableArtTest, DescribeArt_SeatsOnlyTheOccupied)
{
    const TableScene scene;
    auto snapshot = liveTable();
    snapshot.seats.push_back(SeatSnapshot{});

    const auto art = artOf(scene, kCanvas, snapshot);

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

    const auto art = artOf(scene, kCanvas, snapshot);

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

    const auto art = artOf(scene, kCanvas, snapshot);

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

    const auto art = artOf(scene, kCanvas, snapshot);

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

    const auto art = artOf(scene, kCanvas, snapshot);

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

    const auto art = artOf(scene, kCanvas, snapshot);

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
        artOf(scene, Size{.width = 40, .height = 24}, snapshot);

    EXPECT_FALSE(art.empty());
    for (const auto &blit : art)
    {
        EXPECT_GE(blit.destination.origin.x, 0);
        EXPECT_GE(blit.destination.origin.y, 0);
    }
}

// A card is 8 pixels wide at the floor.
// So a full board is 44 across and does not fit a canvas 40 wide.
// The centring subtracts, and these are unsigned.
// Without the guard the board would start at four billion.
TEST(TableArtTest, DescribeArt_LeftAlignsABoardWiderThanTheCanvas)
{
    const TableScene scene;
    auto snapshot = idleTable();
    snapshot.board = parseCards("Ah Kd 7c 2s 9h");

    const auto art =
        artOf(scene, Size{.width = 40, .height = 400}, snapshot);

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

// A picture two runs disagree about is two different tables.
// So every field has to count towards a blit being the same blit.
// A defaulted operator== stops at the first difference it finds.
// So each field is asserted on its own.
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

    const auto art = artOf(scene, kCanvas, snapshot);

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
        .Times(static_cast<int>(artOf(scene, kCanvas, snapshot).size()));

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

// --- One layout, and the art placed from it -------------------------

namespace
{
    using antwika::gfx::Point;
    using antwika::ui::DrawText;
    using antwika::ui::Frame;
    using antwika::ui::WidgetRect;
    using antwika::ui::WidgetRects;
    namespace widgets = antwika::poker::widgets;

    // Awkward on purpose.
    // One ordinary window.
    // One far wider than it is tall, and one far taller than wide.
    // One too small for nine rows, which is where shrinking starts.
    const std::vector<Size> kCanvases{
        Size{.width = 1024, .height = 640},
        Size{.width = 1920, .height = 360},
        Size{.width = 480, .height = 1200},
        Size{.width = 320, .height = 200},
        Size{.width = 800, .height = 600}};

    [[nodiscard]] std::int32_t rightOf(Rect rect) noexcept
    {
        return rect.origin.x
               + static_cast<std::int32_t>(rect.size.width);
    }

    [[nodiscard]] std::int32_t bottomOf(Rect rect) noexcept
    {
        return rect.origin.y
               + static_cast<std::int32_t>(rect.size.height);
    }

    [[nodiscard]] bool overlaps(Rect one, Rect other) noexcept
    {
        return one.origin.x < rightOf(other)
               && other.origin.x < rightOf(one)
               && one.origin.y < bottomOf(other)
               && other.origin.y < bottomOf(one);
    }

    [[nodiscard]] bool contains(Rect outer, Rect inner) noexcept
    {
        return inner.origin.x >= outer.origin.x
               && inner.origin.y >= outer.origin.y
               && rightOf(inner) <= rightOf(outer)
               && bottomOf(inner) <= bottomOf(outer);
    }

    /**
     * @brief One card, and where the layout put it.
     */
    struct CardFace
    {
        std::string text;
        Rect rect;
    };

    /**
     * @brief Every card the frame declared, by the text on it.
     * @param frame The finished frame.
     * @param snapshot The table it was described from.
     * @return One entry per card the layout laid out.
     */
    [[nodiscard]] std::vector<CardFace> cardFacesOf(
        const Frame &frame, const TableSnapshot &snapshot)
    {
        std::vector<CardFace> faces;

        for (std::size_t index = 0; index < snapshot.board.size();
             ++index)
        {
            const auto rect = frame.rects.find(widgets::boardCard(index));

            if (rect.has_value())
            {
                faces.push_back(CardFace{
                    .text = antwika::holdem::toString(
                        snapshot.board[index]),
                    .rect = *rect});
            }
        }

        for (std::size_t seat = 0; seat < snapshot.seats.size(); ++seat)
        {
            if (!snapshot.seats[seat].inHand)
            {
                continue;
            }

            const auto first = widgets::firstHoleCard(seat);
            const auto &cards = snapshot.seats[seat].holeCards;

            for (std::size_t card = 0; card < cards.size(); ++card)
            {
                const auto rect =
                    frame.rects.find(widgets::after(first, card));

                if (rect.has_value())
                {
                    faces.push_back(CardFace{
                        .text = antwika::holdem::toString(cards[card]),
                        .rect = *rect});
                }
            }
        }

        return faces;
    }

    /**
     * @brief Find where a line of text was drawn.
     * @param frame The finished frame to search.
     * @param text The exact line to look for.
     * @return The area its glyphs cover, or nothing when this frame had
     * no room to draw it at all.
     */
    [[nodiscard]] std::optional<Rect> textDrawn(
        const Frame &frame, const std::string &text)
    {
        for (const auto &command : frame.commands)
        {
            const auto *drawn = std::get_if<DrawText>(&command);

            if (drawn != nullptr && drawn->text == text)
            {
                return Rect{
                    .origin = drawn->origin,
                    .size = antwika::gfx::textSize(
                        drawn->text, drawn->scale)};
            }
        }

        return {};
    }

    /**
     * @brief A table mid-hand, every card of it different.
     *
     * Distinct so that a card's text names exactly one card, which is
     * what lets a test say which card a line of text belongs to.
     *
     * @param seats How many seats to sit down.
     * @param stage Which street the hand is on.
     * @return The snapshot.
     */
    [[nodiscard]] TableSnapshot dealtTable(
        std::size_t seats, Stage stage)
    {
        const auto board = parseCards("Ah Kh Qh Jh Th");
        const auto deck = parseCards(
            "As Ks Qs Js Ts 9s 8s 7s 6s 5s 4s 3s 2s Ac Kc Qc Jc Tc");

        TableSnapshot snapshot{
            .tableName = "Antwika",
            .board = board,
            .pot = 240,
            .blinds = {.small = 5, .big = 10},
            .stage = stage,
            .handsPlayed = 12,
            .handInProgress = true,
        };

        for (std::size_t index = 0; index < seats; ++index)
        {
            auto seat = player("p" + std::to_string(index), 100 + index);
            seat.inHand = true;
            seat.holeCards = {deck[2 * index], deck[(2 * index) + 1]};
            seat.isButton = index == 0;
            seat.isToAct = index == 1;
            seat.roundCommitted = index == 1 ? 40 : 0;
            snapshot.seats.push_back(seat);
        }

        return snapshot;
    }
} // namespace

// The whole point of the exercise.
// A rank and a suit belong to one card.
// A picture drawing them anywhere else is of a different hand.
// That held for neither the board nor a seat before this.
// The art and the ui each worked out where a card went.
// The seats came out a row apart.
// The board's text straddled the gap between two cards.
TEST(TableAlignmentTest, EveryCardsTextIsOnTheCardItBelongsTo)
{
    const TableScene scene;
    std::size_t checked = 0;

    for (const auto canvas : kCanvases)
    {
        for (const std::size_t seats : {2U, 3U, 6U, 9U})
        {
            for (const auto stage : {Stage::Flop, Stage::Showdown})
            {
                SCOPED_TRACE(
                    std::to_string(canvas.width) + "x"
                    + std::to_string(canvas.height) + ", "
                    + std::to_string(seats) + " seats");

                const auto snapshot = dealtTable(seats, stage);
                const auto frame = scene.describe(canvas, snapshot);
                const auto faces = cardFacesOf(frame, snapshot);

                ASSERT_EQ(faces.size(), 5 + (2 * seats));

                for (const auto &face : faces)
                {
                    const auto text = textDrawn(frame, face.text);

                    // A row too cramped draws none of the text.
                    // So there is nothing to be in the wrong place.
                    if (!text.has_value())
                    {
                        continue;
                    }

                    ++checked;
                    EXPECT_TRUE(contains(face.rect, *text))
                        << face.text << " is not on its own card";

                    for (const auto &other : faces)
                    {
                        if (other.text == face.text)
                        {
                            continue;
                        }

                        EXPECT_FALSE(overlaps(other.rect, *text))
                            << face.text << " is on " << other.text;
                    }
                }
            }
        }
    }

    // Skipping every card would pass the loop above vacuously.
    EXPECT_GT(checked, 0U);
}

// The same statement from the art's side.
// The blit is not merely near the rectangle the layout gave the card:
// it is that rectangle, because it was read out of the layout.
TEST(TableAlignmentTest, EveryCardIsBlittedIntoTheRectangleItWasGiven)
{
    const TableScene scene;

    const auto face = antwika::poker::sourceOf(
        antwika::poker::kCardFaceSlot);
    const auto back = antwika::poker::sourceOf(
        antwika::poker::kCardBackSlot);

    for (const auto canvas : kCanvases)
    {
        for (const auto stage : {Stage::Flop, Stage::Showdown})
        {
            SCOPED_TRACE(
                std::to_string(canvas.width) + "x"
                + std::to_string(canvas.height));

            const auto snapshot = dealtTable(3, stage);
            const auto frame = scene.describe(canvas, snapshot);
            const auto art =
                scene.describeArt(canvas, frame.rects, snapshot);

            for (const auto &card : cardFacesOf(frame, snapshot))
            {
                bool found = false;
                for (const auto &blit : art)
                {
                    const auto isCard =
                        blit.source == face || blit.source == back;
                    found = found
                            || (isCard
                                && blit.destination == card.rect);
                }

                EXPECT_TRUE(found)
                    << "no card was blitted where " << card.text
                    << " was laid out";
            }
        }
    }
}

// The plate is the seat row, so it cannot be a row out of step with it.
TEST(TableAlignmentTest, EverySeatsPlateStaysInsideTheRowItPlates)
{
    const TableScene scene;
    const auto plate =
        antwika::poker::sourceOf(antwika::poker::kPlateSlot);

    for (const auto canvas : kCanvases)
    {
        SCOPED_TRACE(
            std::to_string(canvas.width) + "x"
            + std::to_string(canvas.height));

        const auto snapshot = dealtTable(6, Stage::Flop);
        const auto frame = scene.describe(canvas, snapshot);
        const auto art =
            scene.describeArt(canvas, frame.rects, snapshot);

        std::vector<Rect> plates;
        for (const auto &blit : art)
        {
            if (blit.source == plate)
            {
                plates.push_back(blit.destination);
            }
        }

        ASSERT_EQ(plates.size(), snapshot.seats.size());

        for (std::size_t index = 0; index < plates.size(); ++index)
        {
            const auto row = frame.rects.find(widgets::seat(index));

            ASSERT_TRUE(row.has_value());
            EXPECT_TRUE(contains(*row, plates[index]));
        }
    }
}

// An id this frame did not declare draws nothing at all.
// A rectangle of its own would be the second layout again.
// So there is no fallback.
TEST(TableArtTest, DescribeArt_DrawsOnlyFeltForAFrameThatNamedNothing)
{
    const TableScene scene;
    const auto snapshot = dealtTable(3, Stage::Showdown);

    const auto art = scene.describeArt(kCanvas, WidgetRects{}, snapshot);

    const auto felt = antwika::poker::sourceOf(antwika::poker::kFeltSlot);
    EXPECT_EQ(blitsOf(art, felt), art.size());
}

// A seat can be declared without its cards being.
// A hand that is over declares no hole cards.
// The plate is still the seat's.
TEST(TableArtTest, DescribeArt_PlatesASeatWhoseCardsWereNotDeclared)
{
    const TableScene scene;
    const auto snapshot = dealtTable(2, Stage::Showdown);

    WidgetRects rects;
    for (std::size_t index = 0; index < snapshot.seats.size(); ++index)
    {
        rects.entries.push_back(WidgetRect{
            .id = widgets::seat(index),
            .rect =
                Rect{
                    .origin =
                        {.x = 0,
                         .y = static_cast<std::int32_t>(index * 80)},
                    .size = {.width = 400, .height = 80}}});
    }

    const auto art = scene.describeArt(kCanvas, rects, snapshot);

    EXPECT_EQ(
        blitsOf(art, antwika::poker::sourceOf(antwika::poker::kPlateSlot)),
        snapshot.seats.size());
    EXPECT_EQ(
        blitsOf(
            art, antwika::poker::sourceOf(antwika::poker::kCardFaceSlot)),
        0U);
    EXPECT_EQ(
        blitsOf(
            art, antwika::poker::sourceOf(antwika::poker::kChipSlot)),
        0U);
}

// A row narrower than the inset it is pulled in by has no plate left.
// These are unsigned, so the alternative is a plate four billion wide.
TEST(TableArtTest, DescribeArt_PlatesNothingIntoARowWithNoRoomLeft)
{
    const TableScene scene;
    const auto snapshot = dealtTable(1, Stage::Flop);

    const WidgetRects rects{
        .entries = {antwika::ui::WidgetRect{
            .id = widgets::seat(0),
            .rect =
                Rect{
                    .origin = {.x = 0, .y = 0},
                    .size = {.width = 4, .height = 40}}}}};

    const auto art = scene.describeArt(kCanvas, rects, snapshot);

    for (const auto &blit : art)
    {
        EXPECT_LE(blit.destination.size.width, kCanvas.width);
    }
}

