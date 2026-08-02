#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "TestTranslator.hpp"
#include "WidgetPixel.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/MenuItem.hpp"
#include "antwika/game/Toolbar.hpp"

using antwika::game::tests::kTranslator;

using antwika::game::Camera;
using antwika::game::CityRatings;
using antwika::game::Toolbar;
using antwika::gfx::Point;
using antwika::gfx::Rect;
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
    // So a test asks the layout rather than sweeping the canvas for it.
    [[nodiscard]] std::optional<Point> pointOn(WidgetId id)
    {
        const Toolbar toolbar{kTranslator};
        const Camera camera;

        return antwika::game::tests::widgetCentre(
            toolbar.describe(kCanvas, Pointer{}, camera), id);
    }

    [[nodiscard]] Rect rectOf(const antwika::ui::Frame &frame, WidgetId id)
    {
        const auto found = frame.rects.find(id);

        return found.value_or(Rect{});
    }

    [[nodiscard]] bool encloses(Rect outer, Rect inner) noexcept
    {
        const auto right = [](Rect box)
        {
            return box.origin.x
                   + static_cast<std::int32_t>(box.size.width);
        };
        const auto bottom = [](Rect box)
        {
            return box.origin.y
                   + static_cast<std::int32_t>(box.size.height);
        };

        return inner.origin.x >= outer.origin.x
               && inner.origin.y >= outer.origin.y
               && right(inner) <= right(outer)
               && bottom(inner) <= bottom(outer);
    }

    // What the three pieces leave in the middle.
    // Off the layout, rather than from constants beside it.
    [[nodiscard]] Rect gameViewOf(const antwika::ui::Frame &frame)
    {
        const auto top = rectOf(frame, widgets::kTopBar);
        const auto side = rectOf(frame, widgets::kSidePanel);
        const auto bottom = rectOf(frame, widgets::kBottomBar);

        const auto y =
            top.origin.y + static_cast<std::int32_t>(top.size.height);

        return Rect{
            .origin = {.x = 0, .y = y},
            .size = {
                .width = static_cast<std::uint32_t>(side.origin.x),
                .height =
                    static_cast<std::uint32_t>(bottom.origin.y - y)}};
    }
} // namespace

TEST(ToolbarTest, Describe_DrawsEveryButtonAndTheZoomItIsAt)
{
    const Toolbar toolbar{kTranslator};
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
    const Toolbar toolbar{kTranslator};
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
    const Toolbar toolbar{kTranslator};
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
    const Toolbar toolbar{kTranslator};
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
    const Toolbar toolbar{kTranslator};
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
    const Toolbar toolbar{kTranslator};
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
TEST(ToolbarTest, Describe_CoversNoneOfTheGameView)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;
    const auto view =
        gameViewOf(toolbar.describe(kCanvas, Pointer{}, camera));

    // The far corner of the middle band.
    // Which is the pixel nearest to all three pieces at once.
    const Point corner{
        .x = view.origin.x
             + static_cast<std::int32_t>(view.size.width) - 1,
        .y = view.origin.y
             + static_cast<std::int32_t>(view.size.height) - 1};

    for (const auto at : {view.origin, corner})
    {
        const auto frame = toolbar.describe(
            kCanvas,
            Pointer{.position = at, .down = true, .pressed = true},
            camera);

        EXPECT_FALSE(frame.interactions.pointerOverUi);
        EXPECT_EQ(kNoWidget, frame.interactions.activated);
    }
}

// Nothing here can clip, so the layout has to do the containing.
TEST(ToolbarTest, Describe_KeepsEveryWidgetInsideTheCanvas)
{
    const Toolbar toolbar{kTranslator};
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

// A rating is simulation state, read back out where it can be seen.
// A pure function of the World, so a replay draws the same bar.
TEST(ToolbarTest, Describe_ReportsTheRatingsItIsGiven)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;

    const auto frame = toolbar.describe(
        kCanvas,
        Pointer{},
        camera,
        antwika::game::BuildTool::Road,
        false,
        0,
        CityRatings{.population = 42, .employment = 75});

    EXPECT_THAT(
        textsOf(frame.commands),
        ::testing::IsSupersetOf(
            {std::string{"pop 42"}, std::string{"jobs 75%"}}));
}

// A picture, and only a picture.
// There is nothing here to press, so the labels declare no widget.
// And so a rating can never become an input.
TEST(ToolbarTest, Describe_DeclaresNoWidgetForARating)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;

    const auto without = toolbar.describe(kCanvas, Pointer{}, camera);
    const auto with = toolbar.describe(
        kCanvas,
        Pointer{},
        camera,
        antwika::game::BuildTool::Road,
        false,
        0,
        CityRatings{.population = 42, .employment = 75});

    std::vector<WidgetId> before;
    std::vector<WidgetId> after;

    for (const auto &entry : without.rects.entries)
    {
        before.push_back(entry.id);
    }

    for (const auto &entry : with.rects.entries)
    {
        after.push_back(entry.id);
    }

    EXPECT_EQ(after, before);
}

// The whole reason they go after the menu rather than beside the zoom.
// Every widget declared before them keeps its rectangle.
// So a session recorded before this replays onto the same buttons.
TEST(ToolbarTest, Describe_LeavesEveryExistingWidgetWhereItWas)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;

    const auto without = toolbar.describe(kCanvas, Pointer{}, camera);
    const auto with = toolbar.describe(
        kCanvas,
        Pointer{},
        camera,
        antwika::game::BuildTool::Road,
        false,
        0,
        CityRatings{
            .population = 1234,
            .employment = 99,
            .averageHousingLevel = 250,
            .serviceReach = 50});

    for (const auto id : {
             widgets::kZoomOut,
             widgets::kZoomIn,
             widgets::kResetView,
             widgets::kPauseResume,
             widgets::kMenu,
             widgets::toolWidget(antwika::game::BuildTool::Road),
             widgets::toolWidget(antwika::game::BuildTool::EngineerPost)})
    {
        const auto before =
            antwika::game::tests::widgetCentre(without, id);
        const auto after = antwika::game::tests::widgetCentre(with, id);

        ASSERT_TRUE(before.has_value());
        ASSERT_TRUE(after.has_value());
        EXPECT_EQ(*after, *before);
    }
}

// Every palette button is on the panel down the right.
// So none of them is over the city.
// Which is what keeps a press meant for a button off a cell.
TEST(ToolbarTest, Describe_KeepsEveryPaletteButtonOnTheSidePanel)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;
    const auto frame = toolbar.describe(kCanvas, Pointer{}, camera);
    const auto panel = rectOf(frame, widgets::kSidePanel);
    const auto view = gameViewOf(frame);

    for (std::size_t index = 0; index < antwika::game::kBuildToolCount;
         ++index)
    {
        const auto id = widgets::toolWidget(
            static_cast<antwika::game::BuildTool>(index));
        const auto button = frame.rects.find(id);

        ASSERT_TRUE(button.has_value());
        EXPECT_TRUE(encloses(panel, *button));
        EXPECT_GE(button->origin.x, view.origin.x
                                        + static_cast<std::int32_t>(
                                            view.size.width));
    }
}

// The view controls and the readouts are along the bottom now.
// The game menu is along the top.
TEST(ToolbarTest, Describe_PutsEveryWidgetOnThePieceItBelongsTo)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;
    const auto frame = toolbar.describe(kCanvas, Pointer{}, camera);

    for (const auto id : {widgets::kGameMenu, widgets::kMenu})
    {
        const auto found = frame.rects.find(id);

        ASSERT_TRUE(found.has_value());
        EXPECT_TRUE(encloses(rectOf(frame, widgets::kTopBar), *found));
    }

    for (const auto id : {
             widgets::kZoomOut,
             widgets::kZoomIn,
             widgets::kResetView,
             widgets::kPauseResume})
    {
        const auto found = frame.rects.find(id);

        ASSERT_TRUE(found.has_value());
        EXPECT_TRUE(encloses(rectOf(frame, widgets::kBottomBar), *found));
    }
}

// A city nobody can see is not worth building one in.
TEST(ToolbarTest, Describe_LeavesMostOfTheCanvasToTheCity)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;
    const auto view =
        gameViewOf(toolbar.describe(kCanvas, Pointer{}, camera));

    EXPECT_GE(view.size.width, 3 * kCanvas.width / 4);
    EXPECT_GE(view.size.height, 3 * kCanvas.height / 4);
}

// A list is drawn only where the caller says it is showing.
// antwika::ui remembers nothing about one -- see ui::DropdownSpec.
TEST(ToolbarTest, Describe_ListsTheGameMenusItemsOnlyWhileItIsOpen)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;

    const auto closed = toolbar.describe(kCanvas, Pointer{}, camera);
    const auto open = toolbar.describe(
        kCanvas,
        Pointer{},
        camera,
        antwika::game::BuildTool::Road,
        false,
        0,
        CityRatings{},
        true);

    EXPECT_THAT(
        textsOf(closed.commands),
        ::testing::Not(::testing::Contains(std::string{"load game"})));
    EXPECT_THAT(
        textsOf(open.commands),
        ::testing::IsSupersetOf(
            {std::string{"new game"},
             std::string{"save game"},
             std::string{"load game"},
             std::string{"main menu"},
             std::string{"world map"}}));
}

// The list is an overlay hung under the box it dropped from.
// So every item has a pixel of its own, shared with nothing.
TEST(ToolbarTest, Describe_HasAPixelForEveryMenuItem)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;
    const auto frame = toolbar.describe(
        kCanvas,
        Pointer{},
        camera,
        antwika::game::BuildTool::Road,
        false,
        0,
        CityRatings{},
        true);

    for (std::size_t index = 0; index < antwika::game::kMenuItemCount;
         ++index)
    {
        const auto id = widgets::menuItemWidget(
            static_cast<antwika::game::MenuItem>(index));
        const auto centre =
            antwika::game::tests::widgetCentre(frame, id);

        ASSERT_TRUE(centre.has_value());

        const auto hit = toolbar.describe(
            kCanvas,
            Pointer{.position = centre},
            camera,
            antwika::game::BuildTool::Road,
            false,
            0,
            CityRatings{},
            true);

        EXPECT_EQ(id, hit.interactions.hovered);
    }
}
