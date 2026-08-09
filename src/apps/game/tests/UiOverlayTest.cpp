#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/HoverTarget.hpp>
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
}

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

    overlay.set(onePicture(), {}, true);

    EXPECT_EQ(onePicture(), overlay.commands());
    EXPECT_TRUE(overlay.pointerOverUi());
}

TEST(UiOverlayTest, Set_ReplacesWhateverWasThereBefore)
{
    UiOverlay overlay(kCanvas);

    overlay.set(onePicture(), {}, true);
    overlay.set({}, {}, false);

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST(UiOverlayTest, CoversPoint_AnswersForTheFillsItWasGiven)
{
    UiOverlay overlay(kCanvas);
    overlay.set(onePicture(), {}, false);

    EXPECT_TRUE(overlay.coversPoint(Point{.x = 1, .y = 2}));
    EXPECT_TRUE(overlay.coversPoint(Point{.x = 3, .y = 5}));

    EXPECT_FALSE(overlay.coversPoint(Point{.x = 4, .y = 5}));
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 3, .y = 6}));

    EXPECT_FALSE(overlay.coversPoint(Point{.x = 0, .y = 4}));
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 2, .y = 1}));
}

TEST(UiOverlayTest, CoversPoint_IsNotThePressFlagInEitherDirection)
{
    UiOverlay overlay(kCanvas);

    overlay.set(onePicture(), {}, true);

    EXPECT_TRUE(overlay.pointerOverUi());
    EXPECT_FALSE(overlay.coversPoint(Point{.x = 500, .y = 400}));

    overlay.set(onePicture(), {}, false);

    EXPECT_FALSE(overlay.pointerOverUi());
    EXPECT_TRUE(overlay.coversPoint(Point{.x = 2, .y = 3}));
}

TEST(UiOverlayTest, CoversPoint_IgnoresEverythingThatIsNotAFill)
{
    UiOverlay overlay(kCanvas);
    overlay.set(
        DrawList{
            antwika::ui::DrawText{
                .origin = {.x = 1, .y = 2},
                .text = "tick 0",
                .scale = 1}},
        {},
        false);

    EXPECT_FALSE(overlay.coversPoint(Point{.x = 1, .y = 2}));
}

TEST(UiOverlayTest, HoverTargets_AreKeptBesideThePicture)
{
    UiOverlay overlay(kCanvas);
    const antwika::ui::HoverTargets targets{
        antwika::ui::HoverTarget{.id = antwika::ui::WidgetId{7}}};

    overlay.set(onePicture(), targets, true);

    ASSERT_EQ(overlay.hoverTargets().size(), 1U);
    EXPECT_EQ(overlay.hoverTargets().front().id, antwika::ui::WidgetId{7});
}

TEST(UiOverlayTest, HoverTargets_AreEmptyUntilSomethingDescribesThem)
{
    const UiOverlay overlay(kCanvas);

    EXPECT_TRUE(overlay.hoverTargets().empty());
}
