#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/game/UiOverlay.hpp"

using antwika::game::UiOverlay;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::DrawList;
using antwika::ui::FillRect;

namespace
{
    constexpr Size kCanvas{.width = 640, .height = 480};

    [[nodiscard]] DrawList onePicture()
    {
        return DrawList{
            FillRect{
                .rect =
                    {.origin = {.x = 1, .y = 2},
                     .size = {.width = 3, .height = 4}},
                .color = Color{.red = 5}}};
    }
} // namespace

TEST(UiOverlayTest, Canvas_ReportsTheSizeItWasBuiltOver)
{
    const UiOverlay overlay(kCanvas);

    EXPECT_EQ(kCanvas, overlay.canvas());
}

TEST(UiOverlayTest, Commands_StartEmptyAndCoverNothing)
{
    const UiOverlay overlay(kCanvas);

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST(UiOverlayTest, Set_HoldsThePictureAndWhetherItIsUnderThePointer)
{
    UiOverlay overlay(kCanvas);

    overlay.set(onePicture(), true);

    EXPECT_EQ(onePicture(), overlay.commands());
    EXPECT_TRUE(overlay.pointerOverUi());
}

TEST(UiOverlayTest, Set_ReplacesWhateverWasThereBefore)
{
    UiOverlay overlay(kCanvas);

    overlay.set(onePicture(), true);
    overlay.set({}, false);

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(overlay.pointerOverUi());
}

// The render side's question, and the reason it is a second one.
// The flag above is the recorded pointer's answer from an old tick.
// This is the layout's answer about wherever the caller asks.
TEST(UiOverlayTest, CoversPoint_AnswersForTheFillsItWasGiven)
{
    UiOverlay overlay(kCanvas);
    overlay.set(onePicture(), false);

    // The fill is 3x4 at (1,2), so it holds x in [1,4) and y in [2,6).
    EXPECT_TRUE(overlay.coversPoint(Point{.x = 1, .y = 2}));
    EXPECT_TRUE(overlay.coversPoint(Point{.x = 3, .y = 5}));

    // Half-open on the far edges, so the corner past it is not covered.
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 4, .y = 5}));
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 3, .y = 6}));

    // Nor is anything short of it, in either direction.
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 0, .y = 4}));
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 2, .y = 1}));
}

// The two answer different questions, so neither implies the other.
TEST(UiOverlayTest, CoversPoint_IsNotThePressFlagInEitherDirection)
{
    UiOverlay overlay(kCanvas);

    // A bar somewhere, and a flag left over from a press over it.
    overlay.set(onePicture(), true);

    EXPECT_TRUE(overlay.pointerOverUi());
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 500, .y = 400}));

    overlay.set(onePicture(), false);

    EXPECT_FALSE(overlay.pointerOverUi());
    EXPECT_TRUE(overlay.coversPoint(Point{.x = 2, .y = 3}));
}

// Text is not a fill, so nothing a label wrote covers the grid.
// Which is the same rule ui::Resolve keeps: a background covers.
TEST(UiOverlayTest, CoversPoint_IgnoresEverythingThatIsNotAFill)
{
    UiOverlay overlay(kCanvas);
    overlay.set(
        DrawList{
            antwika::ui::DrawText{
                .origin = {.x = 1, .y = 2},
                .text = "tick 0",
                .scale = 1}},
        false);

    EXPECT_FALSE(overlay.coversPoint(Point{.x = 1, .y = 2}));
}
