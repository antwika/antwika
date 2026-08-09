#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>


#include "Translators.hpp"
#include "WidgetCentre.hpp"
#include "antwika/game/MenuModalScene.hpp"

using antwika::game::tests::kTranslator;

using antwika::game::MenuModalScene;
using antwika::game::tests::widgetCentre;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace modalWidgets = antwika::game::modalWidgets;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] WidgetId activatedAt(Point at)
    {
        const MenuModalScene scene{kTranslator};

        return scene
            .describe(
                kCanvas,
                Pointer{.position = at, .down = true, .pressed = true})
            .interactions.activated;
    }
}

TEST(MenuModalSceneTest, Describe_WorksWithNoPointerAtAll)
{
    const MenuModalScene scene{kTranslator};

    const auto frame = scene.describe(kCanvas, Pointer{});

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_EQ(frame.interactions.hovered, kNoWidget);
    EXPECT_EQ(frame.interactions.activated, kNoWidget);
}
TEST(MenuModalSceneTest, Describe_CoversEveryCornerWithTheScrim)
{
    const MenuModalScene scene{kTranslator};
    const auto last = Point{
        .x = static_cast<std::int32_t>(kCanvas.width) - 1,
        .y = static_cast<std::int32_t>(kCanvas.height) - 1};

    for (const auto at : {
             Point{.x = 0, .y = 0},
             Point{.x = last.x, .y = 0},
             Point{.x = 0, .y = last.y},
             last,
             Point{.x = last.x / 2, .y = last.y / 2}})
    {
        EXPECT_TRUE(
            scene.describe(kCanvas, Pointer{.position = at})
                .interactions.pointerOverUi);
    }
}

TEST(MenuModalSceneTest, ActivatedAt_ActivatesTheMainMenuItem)
{
    const MenuModalScene scene{kTranslator};
    const auto centre = widgetCentre(
        scene.describe(kCanvas, Pointer{}), modalWidgets::kMainMenu);

    ASSERT_TRUE(centre.has_value());
    EXPECT_EQ(modalWidgets::kMainMenu, activatedAt(*centre));
}

TEST(MenuModalSceneTest, ActivatedAt_ActivatesTheBackItem)
{
    const MenuModalScene scene{kTranslator};
    const auto centre = widgetCentre(
        scene.describe(kCanvas, Pointer{}), modalWidgets::kResume);

    ASSERT_TRUE(centre.has_value());
    EXPECT_EQ(modalWidgets::kResume, activatedAt(*centre));
}

TEST(MenuModalSceneTest, ActivatedAt_APressOnTheScrimActivatesNothing)
{
    EXPECT_EQ(kNoWidget, activatedAt(Point{.x = 4, .y = 4}));
}
