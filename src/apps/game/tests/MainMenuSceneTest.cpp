#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/ui/Pointer.hpp>

#include "TestTranslator.hpp"
#include "WidgetPixel.hpp"
#include "antwika/game/MainMenuScene.hpp"

using antwika::game::tests::kTranslator;
using antwika::game::tests::widgetCentre;

using antwika::game::MainMenuScene;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::ui::Pointer;
namespace menuWidgets = antwika::game::menuWidgets;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};
} // namespace

TEST(MainMenuSceneTest, TheMenuDescribesItselfWithNoPointerAtAll)
{
    const MainMenuScene scene{kTranslator};

    const auto frame = scene.describe(kCanvas, Pointer{});

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_EQ(frame.interactions.hovered, antwika::ui::kNoWidget);
    EXPECT_EQ(frame.interactions.activated, antwika::ui::kNoWidget);
}

// The same canvas and pointer always produce the same picture.
// That is what lets a recorded click resolve to the same item.
TEST(MainMenuSceneTest, TheSameArgumentsProduceTheSamePicture)
{
    const MainMenuScene scene{kTranslator};
    const Pointer pointer{.position = Point{.x = 512, .y = 320}};

    EXPECT_EQ(
        scene.describe(kCanvas, pointer).commands,
        scene.describe(kCanvas, pointer).commands);
}

// Where an item is, is the layout's business.
// So the layout is asked rather than swept for.
// See WidgetPixel.hpp.
// Every item, because Options and Quit share a row.
// A sweep down one column would miss whichever it fell beside.
TEST(MainMenuSceneTest, APressOnAnItemActivatesIt)
{
    const MainMenuScene scene{kTranslator};
    const auto frame = scene.describe(kCanvas, Pointer{});

    for (const auto id :
         {menuWidgets::kNewGame,
          menuWidgets::kLoadGame,
          menuWidgets::kWorldMap,
          menuWidgets::kOptions,
          menuWidgets::kQuit})
    {
        const auto centre = widgetCentre(frame, id);

        ASSERT_TRUE(centre.has_value());

        const Pointer pointer{
            .position = *centre, .down = true, .pressed = true};

        EXPECT_EQ(
            scene.describe(kCanvas, pointer).interactions.activated, id);
    }
}

// A mode owns the whole screen.
// So the menu clears rather than being drawn over what was there.
TEST(MainMenuSceneTest, DrawClearsBeforePaintingThePicture)
{
    const MainMenuScene scene{kTranslator};
    NiceMock<MockRenderer> renderer;

    const ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());

    scene.draw(renderer, scene.describe(kCanvas, Pointer{}).commands);
}
