#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Messages.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui_demo::DemoScene;
    using antwika::ui_demo::DemoState;

    constexpr antwika::ui_demo::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 960, .height = 720};
}

TEST(ShowcasePreviewTest, Draw_WritesTheWidgetShowcase)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "ui-demo",
             .title = "Antwika UI",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                const DemoScene scene(kTranslator);
                const DemoState state;
                const Keyboard keyboard;

                const auto frame =
                    scene.describe(kCanvas, Pointer{}, keyboard, state);
                scene.draw(renderer, frame.commands);
            })
            .empty());
}
