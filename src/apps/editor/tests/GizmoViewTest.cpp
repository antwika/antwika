#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/testing/ScratchDirectory.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Interactions.hpp>

#include "antwika/editor/fakes/CanvasPoints.hpp"
#include "antwika/editor/fakes/FakeEditSteps.hpp"
#include "antwika/editor/fakes/FakeNotices.hpp"
#include "antwika/editor/fakes/ViewHarness.hpp"
#include "antwika/editor/ui/GizmoSheet.hpp"
#include "antwika/editor/ui/GizmoView.hpp"
#include "antwika/editor/ui/IconSheet.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::GizmoKind;
using antwika::editor::GizmoView;
using antwika::editor::View;
using antwika::editor::fakes::FakeEditSteps;
using antwika::editor::fakes::FakeNotices;
using antwika::editor::fakes::ViewHarness;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    class GizmoViewTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        FakeEditSteps steps;
        FakeNotices notices;
        ViewHarness harness{logger, steps, notices};
        GizmoView view;
    };

}

TEST_F(GizmoViewTest, Claims_TakesTheGizmosTabOutsidePlay)
{
    EXPECT_TRUE(view.claims(View::Gizmos, false));
    EXPECT_FALSE(view.claims(View::Gizmos, true));
    EXPECT_FALSE(view.claims(View::Icons, false));
}

TEST_F(GizmoViewTest, GetStatusText_NamesTheHandOfTheView)
{
    EXPECT_THAT(
        view.getStatusText(harness.contextNow()),
        ::testing::StartsWith("6 gizmos"));
}

TEST_F(GizmoViewTest, GetBlankGizmoSheet_HoldsAnEmptyCellForEveryKind)
{
    const auto sheet = antwika::editor::getBlankGizmoSheet();

    EXPECT_EQ(
        sheet.size.width,
        antwika::editor::kIconCellSize.width
            * antwika::enums::kCount<GizmoKind>);
    EXPECT_EQ(sheet.size.height, antwika::editor::kIconCellSize.height);

    for (std::size_t slot = 0;
         slot < antwika::enums::kCount<GizmoKind>;
         ++slot)
    {
        EXPECT_FALSE(antwika::editor::isGizmoDrawn(sheet, slot));
    }
}

TEST_F(GizmoViewTest, GetLoadGizmoSheet_FallsBackToABlankSheet)
{
    const antwika::testing::ScratchDirectory scratch("gizmo-sheet-blank");

    EXPECT_EQ(
        antwika::editor::getLoadGizmoSheet(
            scratch.pathIn("maps/no-such-map.json"), "test"),
        antwika::editor::getBlankGizmoSheet());
}

TEST_F(GizmoViewTest, GetLoadGizmoSheet_GrowsANarrowerSheetWithBlankCells)
{
    const antwika::testing::ScratchDirectory scratch("gizmo-sheet-grow");
    const auto mapPath = scratch.pathIn("maps/pad.json");

    std::filesystem::create_directories(scratch.pathIn("textures"));

    auto narrowSheet = antwika::editor::getBlankGizmoSheet();

    narrowSheet.size.width = antwika::editor::kIconCellSize.width * 2;
    narrowSheet.pixels.resize(
        static_cast<std::size_t>(narrowSheet.size.width)
        * narrowSheet.size.height * antwika::gfx::kBytesPerPixel);
    antwika::editor::setIconPixel(
        narrowSheet,
        antwika::enums::index(GizmoKind::Exit),
        {.column = 4, .row = 4},
        antwika::gfx::Color{
            .red = 255, .green = 255, .blue = 255, .alpha = 255});
    antwika::image::writePngFile(
        narrowSheet,
        antwika::map::getSharedTexturePath(
            mapPath, antwika::editor::kGizmoSheet),
        "test");

    const auto grownSheet =
        antwika::editor::getLoadGizmoSheet(mapPath, "test");

    EXPECT_EQ(grownSheet.size, antwika::editor::getBlankGizmoSheet().size);
    EXPECT_TRUE(
        antwika::editor::isGizmoDrawn(
            grownSheet, antwika::enums::index(GizmoKind::Exit)));
    EXPECT_FALSE(
        antwika::editor::isGizmoDrawn(
            grownSheet, antwika::enums::index(GizmoKind::Food)));
    EXPECT_FALSE(
        antwika::editor::isGizmoDrawn(
            grownSheet, antwika::enums::index(GizmoKind::Water)));
}

TEST_F(GizmoViewTest, Paint_ReachesTheFoodAndWaterSlots)
{
    view.pick(antwika::enums::index(GizmoKind::Food));
    view.paint(harness.contextNow(), {.column = 2, .row = 2}, false);
    view.pick(antwika::enums::index(GizmoKind::Water));
    view.paint(harness.contextNow(), {.column = 9, .row = 9}, false);

    EXPECT_TRUE(
        antwika::editor::isGizmoDrawn(
            harness.gizmos.sheetBitmap,
            antwika::enums::index(GizmoKind::Food)));
    EXPECT_TRUE(
        antwika::editor::isGizmoDrawn(
            harness.gizmos.sheetBitmap,
            antwika::enums::index(GizmoKind::Water)));
}

TEST_F(GizmoViewTest, Paint_ColorsThePixelAndLeavesTheSheetUnsaved)
{
    view.pick(antwika::enums::index(GizmoKind::Exit));
    view.paint(harness.contextNow(), {.column = 3, .row = 4}, false);

    EXPECT_EQ(
        antwika::editor::getIconPixelColor(
            harness.gizmos.sheetBitmap,
            antwika::enums::index(GizmoKind::Exit),
            {.column = 3, .row = 4})
            .alpha,
        255);
    EXPECT_TRUE(harness.gizmos.unsaved);
    EXPECT_TRUE(
        antwika::editor::isGizmoDrawn(
            harness.gizmos.sheetBitmap,
            antwika::enums::index(GizmoKind::Exit)));
}

TEST_F(GizmoViewTest, Paint_ErasingClearsThePixelAgain)
{
    view.pick(0);
    view.paint(harness.contextNow(), {.column = 1, .row = 1}, false);
    view.paint(harness.contextNow(), {.column = 1, .row = 1}, true);

    EXPECT_FALSE(antwika::editor::isGizmoDrawn(harness.gizmos.sheetBitmap, 0));
}

TEST_F(GizmoViewTest, TakeWidgets_ClearWipesTheChosenGizmo)
{
    view.pick(antwika::enums::index(GizmoKind::Spawn));
    view.paint(harness.contextNow(), {.column = 5, .row = 5}, false);

    std::optional<std::string> notice;

    EXPECT_TRUE(
        view.takeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::kGizmoClearWidget},
            harness.contextNow(),
            notice));
    EXPECT_FALSE(
        antwika::editor::isGizmoDrawn(
            harness.gizmos.sheetBitmap,
            antwika::enums::index(GizmoKind::Spawn)));
    EXPECT_TRUE(harness.gizmos.unsaved);
    EXPECT_TRUE(harness.document.isDirty());
}

TEST_F(GizmoViewTest, TakeWidgets_LeavesOtherWidgetsToTheirOwners)
{
    std::optional<std::string> notice;

    EXPECT_FALSE(
        view.takeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::kMarkerRemoveWidget},
            harness.contextNow(),
            notice));
}

TEST_F(GizmoViewTest, ConsumePress_TakesUpTheClickedGizmoCell)
{
    const auto cellPoint = antwika::editor::fakes::getMiddleOf(
        antwika::editor::getIconCellRect(
            antwika::editor::getIconSheetBounds(
                antwika::camera::kCanvasSize),
            antwika::enums::kCount<GizmoKind>,
            1));

    EXPECT_TRUE(
        view.consumePress(
            harness.contextNow(),
            antwika::input::PointerButtonPressed{
                .button = antwika::input::MouseButton::Left,
                .position = antwika::editor::fakes::getPointerPositionAt(
                    harness.viewportRenderer.getViewport(), cellPoint)}));
    EXPECT_EQ(view.getPickedIndex(), std::optional<std::size_t>{1});
}

TEST_F(GizmoViewTest, ConsumePress_PaintsThePixelUnderThePointer)
{
    const auto pixelPoint = antwika::editor::fakes::getMiddleOf(
        antwika::editor::getIconPixelRect(
            antwika::editor::getEditedIconRect(
                antwika::editor::getIconDrawBounds(
                    antwika::camera::kCanvasSize)),
            {.column = 2, .row = 3}));

    view.pick(0);

    EXPECT_TRUE(
        view.consumePress(
            harness.contextNow(),
            antwika::input::PointerButtonPressed{
                .button = antwika::input::MouseButton::Left,
                .position = antwika::editor::fakes::getPointerPositionAt(
                    harness.viewportRenderer.getViewport(), pixelPoint)}));
    EXPECT_EQ(
        antwika::editor::getIconPixelColor(
            harness.gizmos.sheetBitmap, 0, {.column = 2, .row = 3})
            .alpha,
        255);
    EXPECT_TRUE(harness.document.isDirty());
    EXPECT_TRUE(harness.stroke.active);

    view.trackPointer(harness.contextNow());

    EXPECT_EQ(
        harness.stroke.brushAtCell,
        (std::optional{
            antwika::geometry::GridCell{.column = 2, .row = 3}}));
}

TEST_F(GizmoViewTest, Draw_LaysTheSheetOutWithoutTrouble)
{
    view.pick(0);
    view.draw(harness.contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(view.getPickedIndex(), std::optional<std::size_t>{0});
}
