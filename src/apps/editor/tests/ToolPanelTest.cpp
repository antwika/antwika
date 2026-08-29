#include <gtest/gtest.h>

#include <set>
#include <string>

#include <antwika/io/AssetPath.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/editor/ui/ToolButtonRow.hpp"
#include "antwika/editor/ui/ToolGroup.hpp"
#include "antwika/editor/ui/ToolGroupMembers.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"
#include "antwika/editor/ui/ToolPlacement.hpp"
#include "antwika/editor/ui/ToolPlacementRow.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace
{

    using antwika::voxel::Facing;
    using antwika::editor::getFacingWidget;
    using antwika::editor::iconOf;
    using antwika::voxel::kEveryKind;
    using antwika::editor::kEveryPaint;
    using antwika::editor::kEveryToolButton;
    using antwika::editor::kIconSide;
    using antwika::editor::getKindWidget;
    using antwika::editor::kMarkedFacings;
    using antwika::editor::kMarkedStairHalves;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;
    using antwika::editor::getLevelWidget;
    using antwika::editor::Paint;
    using antwika::editor::getPaintWidget;
    using antwika::editor::ToolButton;
    using antwika::editor::getToolWidget;
    using antwika::input::Key;

    TEST(ToolPanelTest, IconOf_GivesEveryButtonACellOfItsOwn)
    {
        std::set<std::int32_t> lefts;

        const auto check =
            [&lefts](const antwika::gfx::Rect cell)
        {
            EXPECT_EQ(
                cell.size.width,
                static_cast<std::uint32_t>(kIconSide));
            EXPECT_EQ(
                cell.size.height,
                static_cast<std::uint32_t>(kIconSide));
            EXPECT_EQ(cell.originPoint.y, 0);
            EXPECT_TRUE(lefts.insert(cell.originPoint.x).second);
        };

        for (const auto which : kEveryToolButton)
        {
            check(iconOf(which));
        }

        for (const auto which : kEveryPaint)
        {
            check(iconOf(which));
        }

        for (const auto which : kEveryKind)
        {
            check(iconOf(which));
        }

        for (const auto which : kMarkedFacings)
        {
            check(iconOf(which));
        }

        for (const auto which : kMarkedStairHalves)
        {
            check(iconOf(which));
        }

        EXPECT_EQ(lefts.size(), 29U);
    }

    TEST(ToolPanelTest, IconOf_LaysTheCellsInOneRunWithNoGaps)
    {
        std::set<std::int32_t> lefts;

        for (const auto which : kEveryToolButton)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kEveryPaint)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kEveryKind)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kMarkedFacings)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kMarkedStairHalves)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        std::int32_t expectedCount = 0;

        for (const auto left : lefts)
        {
            EXPECT_EQ(left, expectedCount);

            expectedCount += static_cast<std::int32_t>(kIconSide);
        }
    }

    TEST(ToolPanelTest, Widgets_GiveEveryButtonAWidgetOfItsOwn)
    {
        std::set<antwika::widget::WidgetId> seenWidgets;

        for (const auto which : kEveryToolButton)
        {
            EXPECT_TRUE(seenWidgets.insert(getToolWidget(which)).second);
        }

        for (const auto which : kEveryPaint)
        {
            EXPECT_TRUE(seenWidgets.insert(getPaintWidget(which)).second);
        }

        for (const auto which : kEveryKind)
        {
            EXPECT_TRUE(seenWidgets.insert(getKindWidget(which)).second);
        }

        for (const auto which : kMarkedFacings)
        {
            EXPECT_TRUE(seenWidgets.insert(getFacingWidget(which)).second);
        }

        for (const auto which : kMarkedStairHalves)
        {
            EXPECT_TRUE(seenWidgets.insert(getLevelWidget(which)).second);
        }

        EXPECT_EQ(seenWidgets.size(), 29U);
        EXPECT_FALSE(seenWidgets.contains(antwika::widget::kNoWidget));
        EXPECT_FALSE(
            seenWidgets.contains(antwika::editor::kToolPanelWidget));
    }

    TEST(ToolPanelTest, KindFor_BindsEachKindToAKey)
    {
        using antwika::editor::kindFor;

        EXPECT_EQ(kindFor(Key::N, false), Kind::Normal);
        EXPECT_EQ(kindFor(Key::R, false), Kind::Ramp);

        EXPECT_FALSE(kindFor(Key::W, false).has_value());
        EXPECT_FALSE(kindFor(Key::N, true).has_value());
    }


    TEST(ToolPanelTest, PaintFor_BindsEachDrawingToolToAKey)
    {
        using antwika::editor::paintFor;

        EXPECT_EQ(paintFor(Key::B, false), Paint::Brush);
        EXPECT_EQ(paintFor(Key::L, false), Paint::Line);
        EXPECT_EQ(paintFor(Key::F, false), Paint::Fill);
        EXPECT_EQ(paintFor(Key::M, false), Paint::Select);
        EXPECT_FALSE(paintFor(Key::G, false).has_value());
    }

    TEST(ToolPanelTest, Icons_HoldACellForEveryButtonDrawn)
    {
        const auto sheet = antwika::image::getReadPngFile(
            antwika::io::getAssetPath(
                std::string("icons-16.png")),
            "antwika_editor_tests");
        const auto cells =
            kEveryToolButton.size() + kEveryPaint.size()
            + kEveryKind.size() + kMarkedFacings.size()
            + kMarkedStairHalves.size() + 1;

        EXPECT_EQ(
            sheet.size.width,
            cells * static_cast<std::size_t>(kIconSide));
        EXPECT_EQ(
            sheet.size.height,
            static_cast<std::size_t>(kIconSide));
    }

}

TEST(ToolPanelTest, ToolButtonActive_LightsTheButtonForTheChosenTool)
{
    using antwika::voxel::Kind;

    for (const auto row : antwika::editor::kToolButtonRows)
    {
        EXPECT_TRUE(
            antwika::editor::isToolButtonActive(
                row.button,
                row.tool,
                row.kind.value_or(Kind::Normal)));
    }
}

TEST(ToolPanelTest, ToolButtonActive_LeavesTheOtherToolButtonsDark)
{
    using antwika::editor::Tool;
    using antwika::voxel::Kind;

    for (const auto row : antwika::editor::kToolButtonRows)
    {
        if (row.tool == Tool::Picker)
        {
            continue;
        }

        EXPECT_FALSE(
            antwika::editor::isToolButtonActive(
                row.button, Tool::Picker, Kind::Normal));
    }
}

TEST(ToolPanelTest, ToolButtonActive_TellsTheCubeButtonsApartByTheirKind)
{
    using antwika::editor::ToolButton;
    using antwika::editor::Tool;
    using antwika::voxel::Kind;

    EXPECT_TRUE(
        antwika::editor::isToolButtonActive(
            ToolButton::WaterCube, Tool::Brush, Kind::Water));
    EXPECT_FALSE(
        antwika::editor::isToolButtonActive(
            ToolButton::StoneCube, Tool::Brush, Kind::Water));
    EXPECT_FALSE(
        antwika::editor::isToolButtonActive(
            ToolButton::RampCube, Tool::Brush, Kind::Water));
}

TEST(ToolPanelTest, ToolButtonActive_LeavesTheKindOutWhereNoneIsNamed)
{
    using antwika::editor::ToolButton;
    using antwika::editor::Tool;
    using antwika::voxel::Kind;

    for (const auto kind : antwika::voxel::kEveryKind)
    {
        EXPECT_TRUE(
            antwika::editor::isToolButtonActive(
                ToolButton::Rubber, Tool::Eraser, kind));
    }
}

TEST(ToolPanelTest, ToolsIn_SplitsTheButtonsIntoTheirGroups)
{
    using antwika::editor::getToolsIn;
    using antwika::editor::kEveryToolButton;
    using antwika::editor::kToolGroupRows;
    using antwika::editor::ToolButton;
    using antwika::editor::ToolGroup;

    const auto voxelTools = getToolsIn(ToolGroup::Voxel);
    const auto entityTools = getToolsIn(ToolGroup::Entity);

    EXPECT_EQ(
        voxelTools.count + entityTools.count, kEveryToolButton.size());
    EXPECT_EQ(voxelTools.buttons.at(0), ToolButton::StoneCube);
    EXPECT_EQ(
        voxelTools.buttons.at(voxelTools.count - 1), ToolButton::Rubber);
    EXPECT_EQ(entityTools.buttons.at(0), ToolButton::Select);
    EXPECT_EQ(
        entityTools.buttons.at(entityTools.count - 1), ToolButton::Water);

    for (const auto &row : kToolGroupRows)
    {
        EXPECT_FALSE(
            antwika::editor::getToolGroupTitle(row.group).empty());
    }
}

TEST(ToolPanelTest, PlacementOf_SendsEveryMarkerToolToTheMarkerPlacement)
{
    using antwika::editor::placementOf;
    using antwika::editor::ToolPlacement;
    using antwika::editor::Tool;

    for (const auto tool :
         {Tool::Checkpoint,
          Tool::Food,
          Tool::Water})
    {
        EXPECT_EQ(placementOf(tool), ToolPlacement::Marker);
    }

    EXPECT_EQ(placementOf(Tool::Start), ToolPlacement::StartOrExit);
    EXPECT_EQ(placementOf(Tool::Exit), ToolPlacement::StartOrExit);
    EXPECT_EQ(placementOf(Tool::Lamp), ToolPlacement::Lamp);
    EXPECT_EQ(placementOf(Tool::Stamp), ToolPlacement::Stamp);
    EXPECT_EQ(
        placementOf(Tool::Character), ToolPlacement::Character);
}

TEST(ToolPanelTest, PlacementOf_LeavesTheDrawingToolsToTheShapePath)
{
    using antwika::editor::placementOf;
    using antwika::editor::ToolPlacement;
    using antwika::editor::Tool;

    EXPECT_EQ(placementOf(Tool::Brush), ToolPlacement::Shape);
    EXPECT_EQ(placementOf(Tool::Eraser), ToolPlacement::Shape);
    EXPECT_EQ(placementOf(Tool::Picker), ToolPlacement::Shape);
}
