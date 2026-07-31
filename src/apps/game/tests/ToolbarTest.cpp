#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Toolbar.hpp"

using antwika::game::Camera;
using antwika::game::Toolbar;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::textSize;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace widgets = antwika::game::widgets;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] std::vector<std::string> textsOf(const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    // Where a button sits is the layout's business.
    // So a test looks for a pixel that hits the one it means.
    // Stepping by four cannot miss a button several glyphs tall.
    [[nodiscard]] std::optional<Point> pointOn(WidgetId id)
    {
        const Toolbar toolbar;
        const Camera camera;

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(kCanvas.height);
             y += 4)
        {
            for (std::int32_t x = 0;
                 x < static_cast<std::int32_t>(kCanvas.width);
                 x += 4)
            {
                const Pointer pointer{.position = Point{.x = x, .y = y}};
                const auto frame =
                    toolbar.describe(kCanvas, pointer, camera);

                if (frame.interactions.hovered == id)
                {
                    return Point{.x = x, .y = y};
                }
            }
        }

        return std::nullopt;
    }
} // namespace

TEST(ToolbarTest, Describe_DrawsEveryButtonAndTheZoomItIsAt)
{
    const Toolbar toolbar;
    const Camera camera;

    const auto frame = toolbar.describe(kCanvas, Pointer{}, camera);

    EXPECT_THAT(
        textsOf(frame.commands),
        ::testing::IsSupersetOf(
            {std::string{"zoom out"},
             std::string{"zoom in"},
             std::string{"reset view"},
             "zoom " + std::to_string(camera.zoomLevel())}));
}

// Labelled with what pressing it does, so the two states read apart.
TEST(ToolbarTest, Describe_LabelsThePauseButtonFromWhatItWouldDo)
{
    const Toolbar toolbar;
    const Camera camera;

    EXPECT_THAT(
        textsOf(
            toolbar
                .describe(
                    kCanvas,
                    Pointer{},
                    camera,
                    antwika::game::BuildTool::Road,
                    false)
                .commands),
        ::testing::Contains(std::string{"pause"}));

    EXPECT_THAT(
        textsOf(
            toolbar
                .describe(
                    kCanvas,
                    Pointer{},
                    camera,
                    antwika::game::BuildTool::Road,
                    true)
                .commands),
        ::testing::Contains(std::string{"resume"}));
}

// The tick is simulation state, read back out where it can be seen.
TEST(ToolbarTest, Describe_ReportsTheTickItIsGiven)
{
    const Toolbar toolbar;
    const Camera camera;

    EXPECT_THAT(
        textsOf(
            toolbar
                .describe(
                    kCanvas,
                    Pointer{},
                    camera,
                    antwika::game::BuildTool::Road,
                    false,
                    1234)
                .commands),
        ::testing::Contains(std::string{"tick 1234"}));
}

// The picture is a value, so the whole bar is one comparison.
// A frame rate would break this, which is why none is described here.
TEST(ToolbarTest, Describe_IsAPureFunctionOfWhatItIsGiven)
{
    const Toolbar toolbar;
    const Camera camera;

    const auto once = toolbar.describe(
        kCanvas, Pointer{}, camera, antwika::game::BuildTool::Road, true, 7);
    const auto again = toolbar.describe(
        kCanvas, Pointer{}, camera, antwika::game::BuildTool::Road, true, 7);

    EXPECT_EQ(once.commands, again.commands);
    EXPECT_NE(
        once.commands,
        toolbar
            .describe(
                kCanvas,
                Pointer{},
                camera,
                antwika::game::BuildTool::Road,
                false,
                7)
            .commands);
}

TEST(ToolbarTest, Describe_ReportsTheZoomTheCameraIsActuallyAt)
{
    const Toolbar toolbar;
    Camera camera;
    camera.zoomOut();

    const auto frame = toolbar.describe(kCanvas, Pointer{}, camera);

    EXPECT_THAT(
        textsOf(frame.commands),
        ::testing::Contains("zoom " + std::to_string(camera.zoomLevel())));
}

TEST(ToolbarTest, Describe_HasAPixelForEveryButton)
{
    EXPECT_TRUE(pointOn(widgets::kZoomOut).has_value());
    EXPECT_TRUE(pointOn(widgets::kZoomIn).has_value());
    EXPECT_TRUE(pointOn(widgets::kResetView).has_value());
    EXPECT_TRUE(pointOn(widgets::kPauseResume).has_value());
}

TEST(ToolbarTest, Describe_ReportsAPressOnTheButtonUnderThePointer)
{
    const Toolbar toolbar;
    const Camera camera;
    const auto at = pointOn(widgets::kZoomIn);
    ASSERT_TRUE(at.has_value());

    const auto frame = toolbar.describe(
        kCanvas,
        Pointer{.position = at, .down = true, .pressed = true},
        camera);

    EXPECT_EQ(widgets::kZoomIn, frame.interactions.activated);
    EXPECT_TRUE(frame.interactions.pointerOverUi);
}

// The grid is what the rest of the window is for.
TEST(ToolbarTest, Describe_CoversNoneOfTheCanvasAwayFromTheBar)
{
    const Toolbar toolbar;
    const Camera camera;

    const auto frame = toolbar.describe(
        kCanvas,
        Pointer{
            .position = Point{
                .x = static_cast<std::int32_t>(kCanvas.width) - 1,
                .y = static_cast<std::int32_t>(kCanvas.height) - 1},
            .down = true,
            .pressed = true},
        camera);

    EXPECT_FALSE(frame.interactions.pointerOverUi);
    EXPECT_EQ(kNoWidget, frame.interactions.activated);
}

// Nothing here can clip, so the layout has to do the containing.
TEST(ToolbarTest, Describe_KeepsEveryWidgetInsideTheCanvas)
{
    const Toolbar toolbar;
    const Camera camera;

    const auto right = static_cast<std::int32_t>(kCanvas.width);
    const auto bottom = static_cast<std::int32_t>(kCanvas.height);

    for (const auto &command :
         toolbar.describe(kCanvas, Pointer{}, camera).commands)
    {
        if (const auto *fill = std::get_if<FillRect>(&command))
        {
            EXPECT_GE(fill->rect.origin.x, 0);
            EXPECT_GE(fill->rect.origin.y, 0);
            EXPECT_LE(
                fill->rect.origin.x
                    + static_cast<std::int32_t>(fill->rect.size.width),
                right);
            EXPECT_LE(
                fill->rect.origin.y
                    + static_cast<std::int32_t>(fill->rect.size.height),
                bottom);

            continue;
        }

        const auto &text = std::get<DrawText>(command);
        const auto extent = textSize(text.text, text.scale);

        EXPECT_GE(text.origin.x, 0);
        EXPECT_GE(text.origin.y, 0);
        EXPECT_LE(
            text.origin.x + static_cast<std::int32_t>(extent.width), right);
        EXPECT_LE(
            text.origin.y + static_cast<std::int32_t>(extent.height),
            bottom);
    }
}
