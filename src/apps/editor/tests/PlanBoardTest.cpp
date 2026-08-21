#include <gtest/gtest.h>

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "antwika/editor/plan/PlanBoard.hpp"

using antwika::editor::Board;
using antwika::editor::Card;
using antwika::editor::Column;
using antwika::editor::addCard;
using antwika::editor::cardOfWidget;
using antwika::editor::cardsOf;
using antwika::editor::columnName;
using antwika::editor::dropIndex;
using antwika::editor::kEveryColumn;
using antwika::editor::kMaxCardsPerColumn;
using antwika::editor::moveCard;
using antwika::editor::planAddWidget;
using antwika::editor::planCardWidget;
using antwika::editor::planColumnWidget;
using antwika::editor::removeCard;
using antwika::gfx::Rect;

namespace
{

    [[nodiscard]] Board boardOf(
        const std::vector<const char *> &titles)
    {
        Board board;

        for (const auto *title : titles)
        {
            EXPECT_TRUE(
                addCard(
                    board,
                    Column::Todo,
                    Card{.title = title, .body = ""}));
        }

        return board;
    }

    [[nodiscard]] std::vector<std::string> titlesOf(
        const Board &board, const Column whichColumn)
    {
        std::vector<std::string> names;

        for (const auto &card : cardsOf(board, whichColumn))
        {
            names.push_back(card.title);
        }

        return names;
    }

    [[nodiscard]] std::vector<Rect> stackOf(const std::size_t many)
    {
        std::vector<Rect> drawnRects;

        for (std::size_t index = 0; index < many; ++index)
        {
            drawnRects.push_back(
                Rect{
                    .originPoint = {.x = 0,
                               .y = static_cast<std::int32_t>(
                                   index * 10)},
                    .size = {.width = 100, .height = 10}});
        }

        return drawnRects;
    }

}

TEST(PlanBoardTest, ColumnName_NamesEveryColumnAndNothingElse)
{
    EXPECT_EQ(columnName(Column::Todo), "To do");
    EXPECT_EQ(columnName(Column::Doing), "Doing");
    EXPECT_EQ(columnName(Column::Done), "Done");
    EXPECT_TRUE(columnName(static_cast<Column>(9)).empty());
}

TEST(PlanBoardTest, AddCard_PutsACardAtTheFootOfItsColumn)
{
    auto board = boardOf({"first", "second"});

    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"first", "second"}));
    EXPECT_TRUE(cardsOf(board, Column::Doing).empty());
}

TEST(PlanBoardTest, AddCard_TakesNoMoreThanTheColumnHoldsRoomFor)
{
    Board board;

    for (std::size_t index = 0; index < kMaxCardsPerColumn; ++index)
    {
        EXPECT_TRUE(addCard(board, Column::Done, Card{}));
    }

    EXPECT_FALSE(addCard(board, Column::Done, Card{}));
    EXPECT_EQ(cardsOf(board, Column::Done).size(), kMaxCardsPerColumn);
}

TEST(PlanBoardTest, RemoveCard_TakesTheCardStandingAtThatPlace)
{
    auto board = boardOf({"first", "second", "third"});

    EXPECT_TRUE(removeCard(board, Column::Todo, 1));
    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"first", "third"}));
}

TEST(PlanBoardTest, RemoveCard_LeavesAColumnAloneWhereNoCardStands)
{
    auto board = boardOf({"only"});

    EXPECT_FALSE(removeCard(board, Column::Todo, 1));
    EXPECT_FALSE(removeCard(board, Column::Doing, 0));
    EXPECT_EQ(cardsOf(board, Column::Todo).size(), 1U);
}

TEST(PlanBoardTest, MoveCard_CarriesACardToAnotherColumn)
{
    auto board = boardOf({"first", "second"});

    EXPECT_EQ(
        moveCard(board, Column::Todo, 0, Column::Doing, 0),
        std::optional<std::size_t>{0});
    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"second"}));
    EXPECT_EQ(
        titlesOf(board, Column::Doing),
        (std::vector<std::string>{"first"}));
}

TEST(PlanBoardTest, MoveCard_LandsPastTheFootOfTheColumnItComesTo)
{
    auto board = boardOf({"first"});

    EXPECT_TRUE(addCard(
            board,
            Column::Done,
            Card{.title = "kept", .body = ""}));
    EXPECT_EQ(
        moveCard(board, Column::Todo, 0, Column::Done, 9),
        std::optional<std::size_t>{1});
    EXPECT_EQ(
        titlesOf(board, Column::Done),
        (std::vector<std::string>{"kept", "first"}));
}

TEST(PlanBoardTest, MoveCard_CarriesACardUpItsOwnColumn)
{
    auto board = boardOf({"first", "second", "third"});

    EXPECT_EQ(
        moveCard(board, Column::Todo, 2, Column::Todo, 0),
        std::optional<std::size_t>{0});
    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"third", "first", "second"}));
}

TEST(PlanBoardTest, MoveCard_CountsASlotAsTheColumnStoodBeforeTheMove)
{
    auto board = boardOf({"first", "second", "third"});

    EXPECT_EQ(
        moveCard(board, Column::Todo, 0, Column::Todo, 2),
        std::optional<std::size_t>{1});
    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"second", "first", "third"}));
}

TEST(PlanBoardTest, MoveCard_CarriesACardToTheFootOfItsOwnColumn)
{
    auto board = boardOf({"first", "second", "third"});

    EXPECT_EQ(
        moveCard(board, Column::Todo, 0, Column::Todo, 3),
        std::optional<std::size_t>{2});
    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"second", "third", "first"}));
}

TEST(PlanBoardTest, MoveCard_LeavesACardThatWouldNotBudge)
{
    auto board = boardOf({"first", "second"});

    EXPECT_FALSE(
        moveCard(board, Column::Todo, 0, Column::Todo, 0).has_value());
    EXPECT_FALSE(
        moveCard(board, Column::Todo, 0, Column::Todo, 1).has_value());
    EXPECT_EQ(
        titlesOf(board, Column::Todo),
        (std::vector<std::string>{"first", "second"}));
}

TEST(PlanBoardTest, MoveCard_CarriesNothingFromAnEmptyPlace)
{
    auto board = boardOf({"only"});

    EXPECT_FALSE(
        moveCard(board, Column::Todo, 5, Column::Doing, 0).has_value());
    EXPECT_FALSE(
        moveCard(board, Column::Done, 0, Column::Doing, 0).has_value());
}

TEST(PlanBoardTest, MoveCard_TakesNoCardIntoAColumnWithNoRoomLeft)
{
    auto board = boardOf({"first"});

    for (std::size_t index = 0; index < kMaxCardsPerColumn; ++index)
    {
        EXPECT_TRUE(addCard(board, Column::Doing, Card{}));
    }

    EXPECT_FALSE(
        moveCard(board, Column::Todo, 0, Column::Doing, 0).has_value());
    EXPECT_EQ(cardsOf(board, Column::Todo).size(), 1U);
}

TEST(PlanBoardTest, DropIndex_DropsBeforeACardThePointerStandsAbove)
{
    const auto drawnCards = stackOf(3);

    EXPECT_EQ(dropIndex(drawnCards, 0), 0U);
    EXPECT_EQ(dropIndex(drawnCards, 4), 0U);
    EXPECT_EQ(dropIndex(drawnCards, 6), 1U);
    EXPECT_EQ(dropIndex(drawnCards, 16), 2U);
}

TEST(PlanBoardTest, DropIndex_DropsPastTheFootOfTheLastCard)
{
    EXPECT_EQ(dropIndex(stackOf(3), 900), 3U);
}

TEST(PlanBoardTest, DropIndex_DropsAtTheHeadOfAColumnHoldingNothing)
{
    EXPECT_EQ(dropIndex(stackOf(0), 400), 0U);
}

TEST(PlanBoardTest, PlanCardWidget_HandsOutANumberToEveryCard)
{
    std::set<antwika::ui::WidgetId> seenWidgets;

    for (const auto which : kEveryColumn)
    {
        EXPECT_TRUE(seenWidgets.insert(planColumnWidget(which)).second);
        EXPECT_TRUE(seenWidgets.insert(planAddWidget(which)).second);

        for (std::size_t index = 0; index < kMaxCardsPerColumn; ++index)
        {
            EXPECT_TRUE(
                seenWidgets.insert(planCardWidget(which, index)).second);
        }
    }
}

TEST(PlanBoardTest, CardOfWidget_ReadsBackWhateverPlanCardWidgetWrote)
{
    for (const auto which : kEveryColumn)
    {
        for (std::size_t index = 0; index < kMaxCardsPerColumn; ++index)
        {
            EXPECT_EQ(
                cardOfWidget(planCardWidget(which, index)),
                (std::optional{std::pair{which, index}}));
        }
    }
}

TEST(PlanBoardTest, CardOfWidget_ReadsNoCardFromAnotherNumber)
{
    EXPECT_FALSE(cardOfWidget(antwika::ui::kNoWidget).has_value());
    EXPECT_FALSE(
        cardOfWidget(planColumnWidget(Column::Todo)).has_value());
    EXPECT_FALSE(
        cardOfWidget(antwika::ui::WidgetId{9000}).has_value());
}
