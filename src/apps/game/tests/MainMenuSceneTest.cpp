#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/ui/Pointer.hpp>

#include "Translators.hpp"
#include "WidgetCentre.hpp"
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
}

TEST(MainMenuSceneTest, Describe_WorksWithNoPointerAtAll)
{
    const MainMenuScene scene{kTranslator};

    const auto frame = scene.describe(kCanvas, Pointer{});

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_EQ(frame.interactions.hovered, antwika::ui::kNoWidget);
    EXPECT_EQ(frame.interactions.activated, antwika::ui::kNoWidget);
}
TEST(MainMenuSceneTest, Describe_APressOnAnItemActivatesIt)
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

TEST(MainMenuSceneTest, Draw_ClearsBeforePainting)
{
    const MainMenuScene scene{kTranslator};
    NiceMock<MockRenderer> renderer;

    const ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());

    scene.draw(renderer, scene.describe(kCanvas, Pointer{}).commands);
}
