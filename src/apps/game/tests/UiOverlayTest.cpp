#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/game/UiOverlay.hpp"

using antwika::game::UiOverlay;
using antwika::gfx::Color;
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
