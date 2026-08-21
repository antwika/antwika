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
using antwika::editor::planAddWidget;
using antwika::editor::planCardWidget;
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

    [[nodiscard]] std::string boardPath(const std::string &name)
    {
        return (std::filesystem::temp_directory_path()
                / ("antwika-plan-" + name + ".json"))
            .string();
    }

    [[nodiscard]] PlanView emptyBoard(const std::string &name)
    {
        const auto path = boardPath(name);

        std::filesystem::remove(path);

        PlanView view;

        view.open(path);

        return view;
    }

    [[nodiscard]] Interactions pressOn(const antwika::ui::WidgetId whichWidget)
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
    const auto view = emptyBoard("open");

    for (const auto which : antwika::editor::kEveryColumn)
    {
        EXPECT_TRUE(cardsOf(view.board(), which).empty());
    }

    EXPECT_FALSE(view.unsaved());
    EXPECT_FALSE(view.picked());
    EXPECT_FALSE(view.dragging());
}

TEST(PlanViewTest, HandleWidgets_AddsACardAndPicksItToBeWrittenIn)
{
    auto view = emptyBoard("add");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Doing)),
        Point{},
        focusedField,
        notice));

    EXPECT_EQ(cardsOf(view.board(), Column::Doing).size(), 1U);
    EXPECT_EQ(focusedField, FocusedField::PlanTitle);
    EXPECT_TRUE(view.picked());
    EXPECT_TRUE(view.unsaved());
    EXPECT_FALSE(notice.has_value());
}

TEST(PlanViewTest, HandleWidgets_LeavesAPressOnNothingOfItsOwnAlone)
{
    auto view = emptyBoard("stranger");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_FALSE(view.handleWidgets(
        pressOn(antwika::ui::WidgetId{9999}),
        Point{},
        focusedField,
        notice));
    EXPECT_FALSE(view.picked());
}

TEST(PlanViewTest, HandleWidgets_TakesUpACardOfTheBoard)
{
    auto view = emptyBoard("pick");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));
    EXPECT_TRUE(view.handleWidgets(
        pressOn(planCardWidget(Column::Todo, 0)),
        Point{.x = 5, .y = 5},
        focusedField,
        notice));

    EXPECT_EQ(focusedField, FocusedField::Nothing);
    EXPECT_TRUE(view.picked());
}

TEST(PlanViewTest, HandleWidgets_LeavesACardThatIsNotThereAlone)
{
    auto view = emptyBoard("missing");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_FALSE(view.handleWidgets(
        pressOn(planCardWidget(Column::Todo, 2)),
        Point{},
        focusedField,
        notice));
}

TEST(PlanViewTest, HandleWidgets_FocusesTheTitleAndTheBodyOfAPickedCard)
{
    auto view = emptyBoard("focus");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));
    focusedField = FocusedField::Nothing;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(kPlanBodyWidget), Point{}, focusedField, notice));
    EXPECT_EQ(focusedField, FocusedField::PlanBody);
    EXPECT_TRUE(view.handleWidgets(
        pressOn(kPlanTitleWidget), Point{}, focusedField, notice));
    EXPECT_EQ(focusedField, FocusedField::PlanTitle);
}

TEST(PlanViewTest, HandleWidgets_DeletesThePickedCard)
{
    auto view = emptyBoard("delete");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));
    EXPECT_TRUE(view.handleWidgets(
        pressOn(kPlanDeleteWidget), Point{}, focusedField, notice));

    EXPECT_TRUE(cardsOf(view.board(), Column::Todo).empty());
    EXPECT_FALSE(view.picked());
    EXPECT_EQ(notice, std::optional<std::string>{"card deleted"});
}

TEST(PlanViewTest, CarryEdits_WritesWhatWasTypedIntoThePickedCard)
{
    auto view = emptyBoard("edits");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));

    Frame typedFrame;

    typedFrame.interactions.edit =
        TextEdit{.fieldWidget = kPlanTitleWidget, .text = "a title"};
    view.carryEdits(typedFrame, focusedField);

    EXPECT_EQ(cardsOf(view.board(), Column::Todo).front().title, "a title");

    typedFrame.interactions.edit =
        TextEdit{.fieldWidget = kPlanBodyWidget, .text = "a body", .cursor = 3};
    view.carryEdits(typedFrame, focusedField);

    EXPECT_EQ(cardsOf(view.board(), Column::Todo).front().body, "a body");
}

TEST(PlanViewTest, CarryEdits_LeavesTheBoardAloneWithNoCardPicked)
{
    auto view = emptyBoard("noedits");
    auto focusedField = FocusedField::Nothing;
    Frame typedFrame;

    typedFrame.interactions.edit =
        TextEdit{.fieldWidget = kPlanTitleWidget, .text = "a title"};
    view.carryEdits(typedFrame, focusedField);

    EXPECT_FALSE(view.unsaved());
}

TEST(PlanViewTest, DraggedTo_TellsACarryFromAClick)
{
    auto view = emptyBoard("drag");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));
    view.carry(
        PlanDrag{
            .fromColumn = Column::Todo,
            .cardIndex = 0,
            .grabbedAtPoint = Point{.x = 10, .y = 10},
            .moved = false,
            .overColumn = std::nullopt,
            .dropIndex = 0});
    view.draggedTo(Point{.x = 11, .y = 11});

    EXPECT_FALSE(view.dragging());

    view.draggedTo(Point{.x = 40, .y = 40});

    EXPECT_TRUE(view.dragging());
}

TEST(PlanViewTest, LetGo_LeavesTheCardWhereItWasWithNowhereToLandIt)
{
    auto view = emptyBoard("letgo");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));
    view.carry(
        PlanDrag{
            .fromColumn = Column::Todo,
            .cardIndex = 0,
            .grabbedAtPoint = Point{},
            .moved = false,
            .overColumn = std::nullopt,
            .dropIndex = 0});

    EXPECT_FALSE(view.letGo().has_value());
    EXPECT_EQ(cardsOf(view.board(), Column::Todo).size(), 1U);
    EXPECT_FALSE(view.dragging());
}

TEST(PlanViewTest, EndDrag_LetsGoOfWhateverWasBeingCarried)
{
    auto view = emptyBoard("enddrag");

    view.carry(
        PlanDrag{
            .fromColumn = Column::Todo,
            .cardIndex = 0,
            .grabbedAtPoint = Point{},
            .moved = true,
            .overColumn = std::nullopt,
            .dropIndex = 0});

    EXPECT_TRUE(view.dragging());

    view.endDrag();

    EXPECT_FALSE(view.dragging());
}

TEST(PlanViewTest, Layout_LaysEveryColumnOutAndSaysHowWideTheyCame)
{
    auto view = emptyBoard("layout");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));

    const auto frame = laidOut(view);

    EXPECT_FALSE(frame.drawList.empty());
    EXPECT_TRUE(frame.rects
                    .find(antwika::editor::planColumnWidget(
                        Column::Todo))
                    .has_value());

    view.updateFrame(frame, std::nullopt);
}

TEST(PlanViewTest, Save_WritesTheBoardBackAndOnlyWhereItChanged)
{
    auto view = emptyBoard("save");
    auto focusedField = FocusedField::Nothing;
    std::optional<std::string> notice;

    EXPECT_FALSE(view.save().has_value());
    EXPECT_TRUE(view.handleWidgets(
        pressOn(planAddWidget(Column::Todo)), Point{}, focusedField, notice));
    EXPECT_TRUE(view.unsaved());
    EXPECT_FALSE(view.save().has_value());
    EXPECT_FALSE(view.unsaved());

    PlanView readView;

    readView.open(boardPath("save"));

    EXPECT_EQ(cardsOf(readView.board(), Column::Todo).size(), 1U);

    std::filesystem::remove(boardPath("save"));
}
