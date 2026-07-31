#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/MainMenuScene.hpp"

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
    const MainMenuScene scene;

    const auto frame = scene.describe(kCanvas, Pointer{});

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_EQ(frame.interactions.hovered, antwika::ui::kNoWidget);
    EXPECT_EQ(frame.interactions.activated, antwika::ui::kNoWidget);
}

// The same canvas and pointer always produce the same picture, which is
// what lets a recorded click be resolved to the same item.
TEST(MainMenuSceneTest, TheSameArgumentsProduceTheSamePicture)
{
    const MainMenuScene scene;
    const Pointer pointer{.position = Point{.x = 512, .y = 320}};

    EXPECT_EQ(
        scene.describe(kCanvas, pointer).commands,
        scene.describe(kCanvas, pointer).commands);
}

TEST(MainMenuSceneTest, APressOnAnItemActivatesIt)
{
    const MainMenuScene scene;
    bool foundNewGame = false;
    bool foundQuit = false;

    for (std::int32_t y = 0;
         y < static_cast<std::int32_t>(kCanvas.height);
         y += 4)
    {
        const Pointer pointer{
            .position = Point{.x = 512, .y = y},
            .down = true,
            .pressed = true};
        const auto activated =
            scene.describe(kCanvas, pointer).interactions.activated;

        foundNewGame = foundNewGame || activated == menuWidgets::kNewGame;
        foundQuit = foundQuit || activated == menuWidgets::kQuit;
    }

    EXPECT_TRUE(foundNewGame);
    EXPECT_TRUE(foundQuit);
}

// A mode owns the whole screen, so the menu clears rather than being
// drawn over whatever was underneath it.
TEST(MainMenuSceneTest, DrawClearsBeforePaintingThePicture)
{
    const MainMenuScene scene;
    NiceMock<MockRenderer> renderer;

    const ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());

    scene.draw(renderer, scene.describe(kCanvas, Pointer{}).commands);
}
