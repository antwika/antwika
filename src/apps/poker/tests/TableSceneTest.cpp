#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Stage.hpp>

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
