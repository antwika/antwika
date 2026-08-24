#include <gtest/gtest.h>

#include <set>
#include <string>

#include <antwika/io/AssetPath.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/editor/ui/ToolButtonRow.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"
#include "antwika/editor/ui/ToolPlacement.hpp"
#include "antwika/editor/ui/ToolPlacementRow.hpp"
#include "antwika/editor/ui/ToolToggles.hpp"

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
    using antwika::map::Paint;
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

        EXPECT_EQ(lefts.size(), 32U);
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

        EXPECT_EQ(seenWidgets.size(), 32U);
        EXPECT_FALSE(seenWidgets.contains(antwika::widget::kNoWidget));
        EXPECT_FALSE(
            seenWidgets.contains(antwika::editor::kToolPanelWidget));
    }

    TEST(ToolPanelTest, ToolFor_BindsEachToolToAKey)
    {
        using antwika::editor::toolFor;

        EXPECT_EQ(toolFor(Key::B, false, false), ToolButton::Brush);
        EXPECT_EQ(toolFor(Key::I, false, false), ToolButton::Picker);
        EXPECT_EQ(toolFor(Key::F, true, false), ToolButton::FreeLook);
        EXPECT_EQ(toolFor(Key::L, false, false), ToolButton::Lighting);
        EXPECT_FALSE(toolFor(Key::G, false, false).has_value());
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
    using antwika::editor::ToolToggles;
    using antwika::map::Tool;

    for (const auto row : antwika::editor::kToolButtonRows)
    {
        if (!row.tool.has_value())
        {
            continue;
        }

        EXPECT_TRUE(
            antwika::editor::isToolButtonActive(
                row.button, *row.tool, ToolToggles{}));
    }
}

TEST(ToolPanelTest, ToolButtonActive_LeavesTheOtherToolButtonsDark)
{
    using antwika::editor::ToolToggles;
    using antwika::map::Tool;

    for (const auto row : antwika::editor::kToolButtonRows)
    {
        if (!row.tool.has_value() || *row.tool == Tool::Brush)
        {
            continue;
        }

        EXPECT_FALSE(
            antwika::editor::isToolButtonActive(
                row.button, Tool::Brush, ToolToggles{}));
    }
}

TEST(ToolPanelTest, ToolButtonActive_KeepsRuleLinesOffTheToolButtons)
{
    using antwika::editor::ToolButton;
    using antwika::editor::ToolToggles;
    using antwika::map::Tool;

    const ToolToggles ruleLinesToggles{.showRuleLines = true};

    EXPECT_TRUE(
        antwika::editor::isToolButtonActive(
            ToolButton::RuleLines, Tool::Brush, ruleLinesToggles));

    for (const auto button :
         {ToolButton::Key,
          ToolButton::Door,
          ToolButton::Checkpoint,
          ToolButton::Food,
          ToolButton::Water})
    {
        EXPECT_FALSE(
            antwika::editor::isToolButtonActive(
                button, Tool::Brush, ruleLinesToggles));
    }
}

TEST(ToolPanelTest, ToolButtonActive_ReadsEachToggleFromItsOwnFlag)
{
    using antwika::editor::ToolButton;
    using antwika::editor::ToolToggles;
    using antwika::map::Tool;

    EXPECT_TRUE(
        antwika::editor::isToolButtonActive(
            ToolButton::FreeLook, Tool::Brush,
            ToolToggles{.freeLook = true}));
    EXPECT_TRUE(
        antwika::editor::isToolButtonActive(
            ToolButton::Lighting, Tool::Brush,
            ToolToggles{.lighting = true}));
    EXPECT_FALSE(
        antwika::editor::isToolButtonActive(
            ToolButton::FreeLook, Tool::Brush,
            ToolToggles{.lighting = true}));
}

TEST(ToolPanelTest, PlacementOf_SendsEveryGateToolToTheGatePlacement)
{
    using antwika::editor::placementOf;
    using antwika::editor::ToolPlacement;
    using antwika::map::Tool;

    for (const auto tool :
         {Tool::Key,
          Tool::Door,
          Tool::Checkpoint,
          Tool::Food,
          Tool::Water})
    {
        EXPECT_EQ(placementOf(tool), ToolPlacement::Gate);
    }

    EXPECT_EQ(placementOf(Tool::Start), ToolPlacement::StartOrExit);
    EXPECT_EQ(placementOf(Tool::Exit), ToolPlacement::StartOrExit);
    EXPECT_EQ(placementOf(Tool::Lamp), ToolPlacement::Lamp);
    EXPECT_EQ(placementOf(Tool::Stamp), ToolPlacement::Stamp);
    EXPECT_EQ(placementOf(Tool::Figure), ToolPlacement::Figure);
    EXPECT_EQ(
        placementOf(Tool::PressurePlate), ToolPlacement::Plate);
}

TEST(ToolPanelTest, PlacementOf_LeavesTheDrawingToolsToTheShapePath)
{
    using antwika::editor::placementOf;
    using antwika::editor::ToolPlacement;
    using antwika::map::Tool;

    EXPECT_EQ(placementOf(Tool::Brush), ToolPlacement::Shape);
    EXPECT_EQ(placementOf(Tool::Eraser), ToolPlacement::Shape);
    EXPECT_EQ(placementOf(Tool::Picker), ToolPlacement::Shape);
}
