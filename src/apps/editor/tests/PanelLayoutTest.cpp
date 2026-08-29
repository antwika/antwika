#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <variant>
#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/ToolGroup.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/support/DrawListQueries.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::kDrawColumnEdgeWidget;
using antwika::editor::kDrawColumnWidget;
using antwika::editor::kRailEdgeWidget;
using antwika::editor::kToolPanelEdgeWidget;
using antwika::editor::kRailWidget;
using antwika::editor::kSheetPanelWidget;
using antwika::editor::kToolPanelWidget;
using antwika::editor::View;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    constexpr std::uint32_t kSeamRoom = 40;

    [[nodiscard]] antwika::input::Position middleOf(
        const antwika::gfx::Rect &whereRect)
    {
        return antwika::input::Position{
            .x = whereRect.originPoint.x
                 + static_cast<std::int32_t>(
                     whereRect.size.width / 2),
            .y = whereRect.originPoint.y
                 + static_cast<std::int32_t>(
                     whereRect.size.height / 2)};
    }

    class PanelLayoutTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] antwika::ui::Frame frameOf(const View shownView)
        {
            probe.viewChoice().activeView = shownView;
            probe.pumpFrame();

            return probe.layoutUi();
        }

        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };
}

TEST_F(PanelLayoutTest, LayoutUi_LeavesNoGapBesideTheRailInASheetView)
{
    for (const auto shownView :
         {View::Atlases, View::Character, View::Gizmos})
    {
        const auto frame = frameOf(shownView);
        const auto drawRect = frame.rects.getWidgetRect(kDrawColumnWidget);
        const auto railRect = frame.rects.getWidgetRect(kRailWidget);

        ASSERT_TRUE(drawRect.has_value());
        ASSERT_TRUE(railRect.has_value());

        const auto drawEnd =
            drawRect->originPoint.x
            + static_cast<std::int32_t>(drawRect->size.width);

        EXPECT_GT(railRect->originPoint.x, drawEnd);
        EXPECT_LT(
            railRect->originPoint.x - drawEnd,
            static_cast<std::int32_t>(kSeamRoom));
    }
}

TEST_F(PanelLayoutTest, LayoutUi_HangsNoRailOnTheIconsView)
{
    const auto frame = frameOf(View::Icons);

    EXPECT_TRUE(frame.rects.getWidgetRect(kDrawColumnWidget).has_value());
    EXPECT_FALSE(frame.rects.getWidgetRect(kRailWidget).has_value());
}

TEST_F(PanelLayoutTest, LayoutUi_TakesTheDrawingColumnToTheRightEdge)
{
    const auto frame = frameOf(View::Icons);
    const auto drawRect = frame.rects.getWidgetRect(kDrawColumnWidget);

    ASSERT_TRUE(drawRect.has_value());

    const auto drawEnd =
        drawRect->originPoint.x
        + static_cast<std::int32_t>(drawRect->size.width);
    const auto windowWide =
        static_cast<std::int32_t>(probe.getWindowSize().width);

    EXPECT_GT(drawEnd, windowWide - static_cast<std::int32_t>(kSeamRoom));
    EXPECT_LE(drawEnd, windowWide);
}

TEST_F(PanelLayoutTest, LayoutUi_LeavesNoGapBesideTheDrawingColumn)
{
    for (const auto shownView :
         {View::Atlases, View::Character, View::Icons, View::Gizmos})
    {
        const auto frame = frameOf(shownView);
        const auto sheetRect = frame.rects.getWidgetRect(kSheetPanelWidget);
        const auto drawRect = frame.rects.getWidgetRect(kDrawColumnWidget);

        ASSERT_TRUE(sheetRect.has_value());
        ASSERT_TRUE(drawRect.has_value());

        const auto sheetEnd =
            sheetRect->originPoint.x
            + static_cast<std::int32_t>(sheetRect->size.width);

        EXPECT_GT(drawRect->originPoint.x, sheetEnd);
        EXPECT_LT(
            drawRect->originPoint.x - sheetEnd,
            static_cast<std::int32_t>(kSeamRoom));
    }
}

TEST_F(PanelLayoutTest, LayoutUi_KeepsTheWorldGapBesideTheToolColumn)
{
    probe.preferences().tool = antwika::editor::Tool::Lamp;

    const auto frame = frameOf(View::World);
    const auto toolRect = frame.rects.getWidgetRect(kToolPanelWidget);
    const auto railRect = frame.rects.getWidgetRect(kRailWidget);

    ASSERT_TRUE(toolRect.has_value());
    ASSERT_TRUE(railRect.has_value());

    const auto toolEnd =
        toolRect->originPoint.x
        + static_cast<std::int32_t>(toolRect->size.width);

    EXPECT_GT(
        railRect->originPoint.x - toolEnd,
        static_cast<std::int32_t>(kSeamRoom));
}

TEST_F(PanelLayoutTest, LayoutUi_HangsEveryToolButtonOnTheWorldToolColumn)
{
    const auto frame = frameOf(View::World);

    for (const auto which : antwika::editor::kEveryToolButton)
    {
        EXPECT_TRUE(
            frame.rects
                .getWidgetRect(antwika::editor::getToolWidget(which))
                .has_value())
            << static_cast<int>(which);
    }
}

TEST_F(PanelLayoutTest, LayoutUi_TitlesTheWorldToolColumnByGroup)
{
    const auto frame = frameOf(View::World);
    const auto texts = antwika::ui::support::textsOf(frame.drawList);

    for (const auto &row : antwika::editor::kToolGroupRows)
    {
        EXPECT_NE(
            std::ranges::find(texts, std::string(row.title)),
            texts.end())
            << row.title;
    }
}

TEST_F(PanelLayoutTest, LayoutUi_KeepsTheKindAndClimbRowsOffTheWorldView)
{
    const auto frame = frameOf(View::World);

    for (const auto kind : antwika::voxel::kEveryKind)
    {
        EXPECT_FALSE(
            frame.rects
                .getWidgetRect(antwika::editor::getKindWidget(kind))
                .has_value());
    }

    for (const auto facing : antwika::editor::kMarkedFacings)
    {
        EXPECT_FALSE(
            frame.rects
                .getWidgetRect(antwika::editor::getFacingWidget(facing))
                .has_value());
    }
}

TEST_F(PanelLayoutTest, LayoutUi_LeavesTheKindAndClimbRowsOnTheAtlasesView)
{
    const auto frame = frameOf(View::Atlases);

    for (const auto kind : antwika::voxel::kEveryKind)
    {
        EXPECT_TRUE(
            frame.rects
                .getWidgetRect(antwika::editor::getKindWidget(kind))
                .has_value());
    }

    for (const auto facing : antwika::editor::kMarkedFacings)
    {
        EXPECT_TRUE(
            frame.rects
                .getWidgetRect(antwika::editor::getFacingWidget(facing))
                .has_value());
    }
}

TEST_F(PanelLayoutTest, LayoutUi_CountsTheHoveredCubeInWholeCubes)
{
    static_cast<void>(frameOf(View::World));

    probe.pointer.hoveredPosition =
        antwika::voxel::VoxelPosition{.x = 6, .y = 2, .z = 5};

    const auto texts =
        antwika::ui::support::textsOf(probe.layoutUi().drawList);

    EXPECT_THAT(texts, ::testing::Contains(::testing::HasSubstr("3 1 2")));
}

TEST_F(PanelLayoutTest, LayoutUi_HangsTheWorldPanelOnTheWorldViewAlone)
{
    EXPECT_TRUE(
        frameOf(View::World)
            .rects.getWidgetRect(antwika::editor::kWorldPanelWidget)
            .has_value());
    EXPECT_FALSE(
        frameOf(View::Atlases)
            .rects.getWidgetRect(antwika::editor::kWorldPanelWidget)
            .has_value());
    EXPECT_FALSE(
        frameOf(View::Icons)
            .rects.getWidgetRect(antwika::editor::kWorldPanelWidget)
            .has_value());
}

TEST_F(PanelLayoutTest, LayoutUi_KeepsTheWorldPanelOffTheColumnsBesideIt)
{
    const auto frame = frameOf(View::World);
    const auto worldRect =
        frame.rects.getWidgetRect(antwika::editor::kWorldPanelWidget);
    const auto toolRect = frame.rects.getWidgetRect(kToolPanelWidget);
    const auto statusRect =
        frame.rects.getWidgetRect(antwika::editor::kStatusBarWidget);

    ASSERT_TRUE(worldRect.has_value());
    ASSERT_TRUE(toolRect.has_value());
    ASSERT_TRUE(statusRect.has_value());

    EXPECT_GT(worldRect->size.width, 0U);
    EXPECT_GT(worldRect->size.height, 0U);
    EXPECT_GE(
        worldRect->originPoint.x,
        toolRect->originPoint.x
            + static_cast<std::int32_t>(toolRect->size.width));
    EXPECT_LE(
        worldRect->originPoint.y
            + static_cast<std::int32_t>(worldRect->size.height),
        statusRect->originPoint.y);
}

TEST_F(PanelLayoutTest, LayoutUi_LeavesTheWorldPanelTheRoomTheRailSpares)
{
    probe.preferences().tool = antwika::editor::Tool::Lamp;

    const auto frame = frameOf(View::World);
    const auto worldRect =
        frame.rects.getWidgetRect(antwika::editor::kWorldPanelWidget);
    const auto railRect = frame.rects.getWidgetRect(kRailWidget);

    ASSERT_TRUE(worldRect.has_value());
    ASSERT_TRUE(railRect.has_value());

    EXPECT_LE(
        worldRect->originPoint.x
            + static_cast<std::int32_t>(worldRect->size.width),
        railRect->originPoint.x);
}

TEST_F(PanelLayoutTest, LayoutUi_HangsNoSheetColumnsOnTheWorldView)
{
    const auto frame = frameOf(View::World);

    EXPECT_FALSE(
        frame.rects.getWidgetRect(kSheetPanelWidget).has_value());
    EXPECT_FALSE(frame.rects.getWidgetRect(kDrawColumnWidget).has_value());
}

TEST_F(PanelLayoutTest, LayoutUi_HangsAGrabBarOnEachColumnOfASheetView)
{
    const auto sheetFrame = frameOf(View::Atlases);

    EXPECT_TRUE(
        sheetFrame.rects.getWidgetRect(kToolPanelEdgeWidget).has_value());
    EXPECT_TRUE(
        sheetFrame.rects.getWidgetRect(kDrawColumnEdgeWidget).has_value());
    EXPECT_TRUE(
        sheetFrame.rects.getWidgetRect(kRailEdgeWidget).has_value());
}

TEST_F(PanelLayoutTest, LayoutUi_GivesTheToolColumnTheWidthItWasDraggedTo)
{
    probe.preferences().panelSizes.toolWidth = 150;

    const auto frame = frameOf(View::Atlases);
    const auto toolRect = frame.rects.getWidgetRect(kToolPanelWidget);

    ASSERT_TRUE(toolRect.has_value());
    EXPECT_EQ(toolRect->size.width, 150U);
}

TEST_F(PanelLayoutTest, LayoutUi_GivesTheDrawColumnTheWidthItWasDraggedTo)
{
    probe.preferences().panelSizes.inspectWidth = 160;

    const auto frame = frameOf(View::Atlases);
    const auto drawRect = frame.rects.getWidgetRect(kDrawColumnWidget);

    ASSERT_TRUE(drawRect.has_value());
    EXPECT_EQ(drawRect->size.width, 160U);
}

TEST_F(PanelLayoutTest, LayoutUi_GivesTheRailTheWidthItWasDraggedTo)
{
    probe.preferences().panelSizes.railWidth = 200;

    const auto frame = frameOf(View::Atlases);
    const auto railRect = frame.rects.getWidgetRect(kRailWidget);

    ASSERT_TRUE(railRect.has_value());
    EXPECT_EQ(railRect->size.width, 200U);
}

TEST_F(PanelLayoutTest, LayoutUi_BacksTheRailWithThePanelColour)
{
    const auto frame = frameOf(View::Atlases);
    const auto railRect = frame.rects.getWidgetRect(kRailWidget);

    ASSERT_TRUE(railRect.has_value());

    EXPECT_TRUE(
        std::any_of(
            frame.drawList.begin(),
            frame.drawList.end(),
            [&railRect](const antwika::ui::DrawCommand &command)
            {
                const auto *fill =
                    std::get_if<antwika::ui::FillRect>(&command);

                return fill != nullptr && fill->rect == *railRect
                       && fill->color == antwika::editor::kPanelColor;
            }));
}

TEST_F(PanelLayoutTest, LayoutUi_HangsTheGizmoPanelOnTheRail)
{
    const auto frame = frameOf(View::Gizmos);

    EXPECT_TRUE(
        frame.rects.getWidgetRect(antwika::editor::kGizmoClearWidget)
            .has_value());
}

TEST_F(PanelLayoutTest, PointerPressed_TakesUpTheEdgeUnderThePointer)
{
    const auto frame = frameOf(View::Atlases);
    const auto edgeRect = frame.rects.getWidgetRect(kRailEdgeWidget);

    ASSERT_TRUE(edgeRect.has_value());

    probe.pointerPressed(
        antwika::input::PointerButtonPressed{
            .button = antwika::input::MouseButton::Left,
            .position = middleOf(*edgeRect)});

    EXPECT_EQ(probe.pointer.heldEdgeWidget, kRailEdgeWidget);
    EXPECT_GT(probe.preferences().panelSizes.railWidth, 0U);
}

TEST_F(PanelLayoutTest, PointerReleased_LetsTheHeldEdgeGo)
{
    const auto frame = frameOf(View::Atlases);
    const auto edgeRect = frame.rects.getWidgetRect(kRailEdgeWidget);

    ASSERT_TRUE(edgeRect.has_value());

    probe.pointerPressed(
        antwika::input::PointerButtonPressed{
            .button = antwika::input::MouseButton::Left,
            .position = middleOf(*edgeRect)});

    ASSERT_EQ(probe.pointer.heldEdgeWidget, kRailEdgeWidget);

    probe.pointerReleased(
        antwika::input::PointerButtonReleased{
            .button = antwika::input::MouseButton::Left,
            .position = middleOf(*edgeRect)});

    EXPECT_EQ(probe.pointer.heldEdgeWidget, antwika::widget::kNoWidget);
}

TEST_F(PanelLayoutTest, Frame_CarriesTheEdgeOnAsThePointerKeepsMoving)
{
    const auto frame = frameOf(View::Atlases);
    const auto edgeRect = frame.rects.getWidgetRect(kRailEdgeWidget);

    ASSERT_TRUE(edgeRect.has_value());

    probe.pointerPressed(
        antwika::input::PointerButtonPressed{
            .button = antwika::input::MouseButton::Left,
            .position = middleOf(*edgeRect)});

    const auto tookUpWidth = probe.preferences().panelSizes.railWidth;
    const auto draggedTo = middleOf(*edgeRect);

    probe.pointer.pointerInWindow = antwika::gfx::Point{
        .x = draggedTo.x - 60, .y = draggedTo.y};
    probe.pumpFrame();

    EXPECT_GT(probe.preferences().panelSizes.railWidth, tookUpWidth);
}

TEST_F(PanelLayoutTest, LayoutUi_HangsAGrabBarOnTheEntityList)
{
    const auto frame = frameOf(View::World);

    EXPECT_TRUE(
        frame.rects.getWidgetRect(antwika::editor::kEntityListPanelWidget)
            .has_value());
    EXPECT_TRUE(
        frame.rects.getWidgetRect(antwika::editor::kEntityListEdgeWidget)
            .has_value());
}

TEST_F(PanelLayoutTest, LayoutUi_GivesTheEntityListTheWidthItWasDraggedTo)
{
    probe.preferences().panelSizes.entityWidth = 140;

    const auto frame = frameOf(View::World);
    const auto listRect =
        frame.rects.getWidgetRect(antwika::editor::kEntityListPanelWidget);

    ASSERT_TRUE(listRect.has_value());
    EXPECT_EQ(listRect->size.width, 140U);
}
