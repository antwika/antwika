#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/editor/editor/state/FocusedField.hpp"
#include "antwika/editor/editor/state/PlanDrag.hpp"
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/ui/PlanView.hpp"

using antwika::editor::Column;
using antwika::editor::FocusedField;
using antwika::editor::kPlanBodyWidget;
using antwika::editor::kPlanDeleteWidget;
using antwika::editor::kPlanTitleWidget;
using antwika::editor::PlanDrag;
using antwika::editor::PlanView;
using antwika::editor::cardsOf;
using antwika::editor::getPlanAddWidget;
using antwika::editor::getPlanCardWidget;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::Frame;
using antwika::ui::Interactions;
using antwika::ui::TextEdit;
using antwika::ui::Theme;

namespace
{
    constexpr Size kCanvasSize{.width = 640, .height = 360};

    [[nodiscard]] std::string getBoardPath(const std::string &name)
    {
        return (std::filesystem::temp_directory_path()
                / ("antwika-plan-" + name + ".json"))
            .string();
    }

    void openEmptyBoard(PlanView &view, const std::string &name)
    {
        const auto path = getBoardPath(name);

        std::filesystem::remove(path);
        view.open(path);
    }

    [[nodiscard]] Interactions getPressOn(
        const antwika::widget::WidgetId whichWidget)
    {
        return Interactions{.activatedWidget = whichWidget};
    }

    [[nodiscard]] Frame laidOut(PlanView &view)
    {
        Context context{kCanvasSize, Theme{}};

        view.layout(context, FocusedField::Nothing);

        return context.build();
    }
}

TEST(PlanViewTest, Open_ReadsAnEmptyBoardWhereThePathHoldsNone)
{
    PlanView view;
    openEmptyBoard(view, "open");

    for (const auto which : antwika::editor::kEveryColumn)
    {
        EXPECT_TRUE(cardsOf(view.getBoard(), which).empty());
    }

    EXPECT_FALSE(view.isUnsaved());
    EXPECT_FALSE(view.isPicked());
    EXPECT_FALSE(view.isDragging());
}

TEST(PlanViewTest, HandleWidgets_AddsACardAndPicksItToBeWrittenIn)
{
    PlanView view;
    openEmptyBoard(view, "add");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Doing)),
        Point{},
        focusedField,
        notice));

    EXPECT_EQ(cardsOf(view.getBoard(), Column::Doing).size(), 1U);
    EXPECT_EQ(focusedField, FocusedField::PlanTitle);
    EXPECT_TRUE(view.isPicked());
    EXPECT_TRUE(view.isUnsaved());
    EXPECT_FALSE(notice.has_value());
}

TEST(PlanViewTest, HandleWidgets_LeavesAPressOnNothingOfItsOwnAlone)
{
    PlanView view;
    openEmptyBoard(view, "stranger");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_FALSE(view.consumeWidgets(
        getPressOn(antwika::widget::WidgetId{9999}),
        Point{},
        focusedField,
        notice));
    EXPECT_FALSE(view.isPicked());
}

TEST(PlanViewTest, HandleWidgets_TakesUpACardOfTheBoard)
{
    PlanView view;
    openEmptyBoard(view, "pick");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));
    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanCardWidget(Column::Todo, 0)),
        Point{.x = 5, .y = 5},
        focusedField,
        notice));

    EXPECT_EQ(focusedField, FocusedField::Nothing);
    EXPECT_TRUE(view.isPicked());
}

TEST(PlanViewTest, HandleWidgets_LeavesACardThatIsNotThereAlone)
{
    PlanView view;
    openEmptyBoard(view, "missing");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_FALSE(view.consumeWidgets(
        getPressOn(getPlanCardWidget(Column::Todo, 2)),
        Point{},
        focusedField,
        notice));
}

TEST(PlanViewTest, HandleWidgets_FocusesTheTitleAndTheBodyOfAPickedCard)
{
    PlanView view;
    openEmptyBoard(view, "focus");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));
    focusedField = FocusedField::Nothing;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(kPlanBodyWidget), Point{}, focusedField, notice));
    EXPECT_EQ(focusedField, FocusedField::PlanBody);
    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(kPlanTitleWidget), Point{}, focusedField, notice));
    EXPECT_EQ(focusedField, FocusedField::PlanTitle);
}

TEST(PlanViewTest, HandleWidgets_DeletesThePickedCard)
{
    PlanView view;
    openEmptyBoard(view, "delete");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));
    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(kPlanDeleteWidget), Point{}, focusedField, notice));

    EXPECT_TRUE(cardsOf(view.getBoard(), Column::Todo).empty());
    EXPECT_FALSE(view.isPicked());
    EXPECT_EQ(notice, std::optional<std::string>{"card deleted"});
}

TEST(PlanViewTest, CarryEdits_WritesWhatWasTypedIntoThePickedCard)
{
    PlanView view;
    openEmptyBoard(view, "edits");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));

    Frame typedFrame;

    typedFrame.interactions.edit =
        TextEdit{.fieldWidget = kPlanTitleWidget, .text = "a title"};
    view.carryEdits(typedFrame, focusedField);

    EXPECT_EQ(cardsOf(view.getBoard(), Column::Todo).front().title, "a title");

    typedFrame.interactions.edit =
        TextEdit{.fieldWidget = kPlanBodyWidget, .text = "a body", .cursor = 3};
    view.carryEdits(typedFrame, focusedField);

    EXPECT_EQ(cardsOf(view.getBoard(), Column::Todo).front().body, "a body");
}

TEST(PlanViewTest, CarryEdits_LeavesTheBoardAloneWithNoCardPicked)
{
    PlanView view;
    openEmptyBoard(view, "noedits");
    auto focusedField = FocusedField::Nothing;
    Frame typedFrame;

    typedFrame.interactions.edit =
        TextEdit{.fieldWidget = kPlanTitleWidget, .text = "a title"};
    view.carryEdits(typedFrame, focusedField);

    EXPECT_FALSE(view.isUnsaved());
}

TEST(PlanViewTest, DraggedTo_TellsACarryFromAClick)
{
    PlanView view;
    openEmptyBoard(view, "drag");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));
    view.carry(
        PlanDrag{
            .fromColumn = Column::Todo,
            .cardIndex = 0,
            .grabbedAtPoint = Point{.x = 10, .y = 10},
            .moved = false,
            .overColumn = std::nullopt,
            .dropIndex = 0});
    view.draggedTo(Point{.x = 11, .y = 11});

    EXPECT_FALSE(view.isDragging());

    view.draggedTo(Point{.x = 40, .y = 40});

    EXPECT_TRUE(view.isDragging());
}

TEST(PlanViewTest, LetGo_LeavesTheCardWhereItWasWithNowhereToLandIt)
{
    PlanView view;
    openEmptyBoard(view, "letgo");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));
    view.carry(
        PlanDrag{
            .fromColumn = Column::Todo,
            .cardIndex = 0,
            .grabbedAtPoint = Point{},
            .moved = false,
            .overColumn = std::nullopt,
            .dropIndex = 0});

    EXPECT_FALSE(view.letGo().has_value());
    EXPECT_EQ(cardsOf(view.getBoard(), Column::Todo).size(), 1U);
    EXPECT_FALSE(view.isDragging());
}

TEST(PlanViewTest, EndDrag_LetsGoOfWhateverWasBeingCarried)
{
    PlanView view;
    openEmptyBoard(view, "enddrag");

    view.carry(
        PlanDrag{
            .fromColumn = Column::Todo,
            .cardIndex = 0,
            .grabbedAtPoint = Point{},
            .moved = true,
            .overColumn = std::nullopt,
            .dropIndex = 0});

    EXPECT_TRUE(view.isDragging());

    view.endDrag();

    EXPECT_FALSE(view.isDragging());
}

TEST(PlanViewTest, Layout_LaysEveryColumnOutAndSaysHowWideTheyCame)
{
    PlanView view;
    openEmptyBoard(view, "layout");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));

    const auto frame = laidOut(view);

    EXPECT_FALSE(frame.drawList.empty());
    EXPECT_TRUE(frame.rects
                    .getWidgetRect(antwika::editor::getPlanColumnWidget(
                        Column::Todo))
                    .has_value());

    view.updateFrame(frame, std::nullopt);
}

TEST(PlanViewTest, Save_WritesTheBoardBackAndOnlyWhereItChanged)
{
    PlanView view;
    openEmptyBoard(view, "save");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_FALSE(view.save().has_value());
    EXPECT_TRUE(view.consumeWidgets(
        getPressOn(getPlanAddWidget(Column::Todo)), Point{}, focusedField, notice));
    EXPECT_TRUE(view.isUnsaved());
    EXPECT_FALSE(view.save().has_value());
    EXPECT_FALSE(view.isUnsaved());

    PlanView readView;

    readView.open(getBoardPath("save"));

    EXPECT_EQ(cardsOf(readView.getBoard(), Column::Todo).size(), 1U);

    std::filesystem::remove(getBoardPath("save"));
}
