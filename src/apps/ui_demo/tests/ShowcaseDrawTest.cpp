#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Messages.hpp"

namespace
{
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui_demo::DemoScene;
    using antwika::ui_demo::DemoState;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr antwika::ui_demo::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 960, .height = 720};
}

TEST(ShowcaseDrawTest, Draw_DrawsTheWidgetShowcase)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const DemoScene scene(kTranslator);
    const DemoState state;
    const Keyboard keyboard;

    const auto frame =
        scene.describe(kCanvas, Pointer{}, keyboard, state);

    scene.draw(renderer, frame.commands);
}
