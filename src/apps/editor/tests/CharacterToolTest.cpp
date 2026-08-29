#include <gtest/gtest.h>

#include <antwika/editor/editor/state/CharacterTool.hpp>

using antwika::editor::CharacterTool;

TEST(CharacterToolTest, GetChosenCharacter_IsEmptyBeforeAnyChoice)
{
    const CharacterTool tool;

    EXPECT_FALSE(tool.getChosenCharacter(3).has_value());
    EXPECT_FALSE(tool.hasChoice());
}

TEST(CharacterToolTest, Choose_HandsBackTheIndexWithinTheList)
{
    CharacterTool tool;

    tool.choose(1);

    EXPECT_EQ(tool.getChosenCharacter(3), 1U);
    EXPECT_TRUE(tool.hasChoice());
}

TEST(CharacterToolTest, GetChosenCharacter_WithholdsAnIndexBeyondTheList)
{
    CharacterTool tool;

    tool.choose(3);

    EXPECT_FALSE(tool.getChosenCharacter(3).has_value());
    EXPECT_TRUE(tool.hasChoice());
}

TEST(CharacterToolTest, DropChoice_ClearsTheChoice)
{
    CharacterTool tool;

    tool.choose(0);
    tool.dropChoice();

    EXPECT_FALSE(tool.hasChoice());
    EXPECT_FALSE(tool.getChosenCharacter(3).has_value());
}

TEST(CharacterToolTest, IsChosen_MatchesOnlyTheChosenIndex)
{
    CharacterTool tool;

    EXPECT_FALSE(tool.isChosen(0));

    tool.choose(2);

    EXPECT_TRUE(tool.isChosen(2));
    EXPECT_FALSE(tool.isChosen(1));
}

TEST(CharacterToolTest, MarkPlaced_SetsTheMarkUntilTheNextChoice)
{
    CharacterTool tool;

    tool.choose(0);

    EXPECT_FALSE(tool.isPlaced());

    tool.markPlaced();

    EXPECT_TRUE(tool.isPlaced());

    tool.choose(1);

    EXPECT_FALSE(tool.isPlaced());
}

TEST(CharacterToolTest, SetPendingLine_ShowsTheLineItWasHanded)
{
    CharacterTool tool;

    EXPECT_TRUE(tool.getPendingLine().empty());

    tool.setPendingLine("hello there");

    EXPECT_EQ(tool.getPendingLine(), "hello there");
}

TEST(CharacterToolTest, TakePendingLine_HandsTheLineOverAndClearsIt)
{
    CharacterTool tool;

    tool.setPendingLine("a line to say");

    EXPECT_EQ(tool.takePendingLine(), "a line to say");
    EXPECT_TRUE(tool.getPendingLine().empty());
}

TEST(CharacterToolTest, ToggleComponent_OpensOneAndClosesTheOther)
{
    CharacterTool tool;

    EXPECT_FALSE(tool.getOpenComponent().has_value());

    tool.toggleComponent(2);

    EXPECT_EQ(tool.getOpenComponent(), 2U);

    tool.toggleComponent(5);

    EXPECT_EQ(tool.getOpenComponent(), 5U);

    tool.toggleComponent(5);

    EXPECT_FALSE(tool.getOpenComponent().has_value());
}

TEST(CharacterToolTest, ToggleAddList_FlipsAndClosesOnAsk)
{
    CharacterTool tool;

    EXPECT_FALSE(tool.isAddListOpen());

    tool.toggleAddList();

    EXPECT_TRUE(tool.isAddListOpen());

    tool.closeAddList();

    EXPECT_FALSE(tool.isAddListOpen());
}

TEST(CharacterToolTest, BeginValueEdit_HoldsTheWidgetAndTheText)
{
    CharacterTool tool;

    tool.beginValueEdit(antwika::widget::WidgetId{901}, "240");

    EXPECT_EQ(
        tool.getEditingValueWidget(), antwika::widget::WidgetId{901});
    EXPECT_EQ(tool.getPendingValueText(), "240");

    tool.setPendingValueText("7");

    EXPECT_EQ(tool.getPendingValueText(), "7");

    tool.endValueEdit();

    EXPECT_FALSE(tool.getEditingValueWidget().has_value());
    EXPECT_TRUE(tool.getPendingValueText().empty());
}

TEST(CharacterToolTest, Choose_ResetsTheInspectorState)
{
    CharacterTool tool;

    tool.toggleComponent(1);
    tool.toggleAddList();
    tool.beginValueEdit(antwika::widget::WidgetId{901}, "240");

    tool.choose(0);

    EXPECT_FALSE(tool.getOpenComponent().has_value());
    EXPECT_FALSE(tool.isAddListOpen());
    EXPECT_FALSE(tool.getEditingValueWidget().has_value());
    EXPECT_TRUE(tool.getPendingValueText().empty());
}

TEST(CharacterToolTest, DropChoice_ResetsTheInspectorState)
{
    CharacterTool tool;

    tool.choose(0);
    tool.toggleComponent(1);
    tool.toggleAddList();
    tool.beginValueEdit(antwika::widget::WidgetId{901}, "240");

    tool.dropChoice();

    EXPECT_FALSE(tool.getOpenComponent().has_value());
    EXPECT_FALSE(tool.isAddListOpen());
    EXPECT_FALSE(tool.getEditingValueWidget().has_value());
    EXPECT_TRUE(tool.getPendingValueText().empty());
}

TEST(CharacterToolTest, SetInspectorScroll_HoldsTheOffset)
{
    CharacterTool tool;

    EXPECT_EQ(tool.getInspectorScroll(), 0U);

    tool.setInspectorScroll(24);

    EXPECT_EQ(tool.getInspectorScroll(), 24U);
}

TEST(CharacterToolTest, SetInspectorTrackHeld_HoldsTheGrip)
{
    CharacterTool tool;

    EXPECT_FALSE(tool.isInspectorTrackHeld());

    tool.setInspectorTrackHeld(true);

    EXPECT_TRUE(tool.isInspectorTrackHeld());
}

TEST(CharacterToolTest, Choose_ResetsTheInspectorScroll)
{
    CharacterTool tool;

    tool.setInspectorScroll(24);
    tool.setInspectorTrackHeld(true);

    tool.choose(0);

    EXPECT_EQ(tool.getInspectorScroll(), 0U);
    EXPECT_FALSE(tool.isInspectorTrackHeld());
}
