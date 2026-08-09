#include "antwika/game/MainMenuScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        constexpr Color kBackdrop{.red = 10, .green = 12, .blue = 18};

        constexpr std::uint32_t kCardWidth = 260;
    }

    MainMenuScene::MainMenuScene(const Translator &translator)
        : translator(translator)
    {
    }

    Frame MainMenuScene::describe(Size canvas, Pointer pointer) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            const auto screen = ui.column(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(
                    {.width = fixedSize(kCardWidth), .height = kFit});

                ui.label(translator.text(MessageId::MenuTitle));

                ui.button(
                    translator.text(MessageId::MenuNewGame),
                    {.id = menuWidgets::kNewGame, .width = kGrow});

                ui.button(
                    translator.text(MessageId::MenuLoadGame),
                    {.id = menuWidgets::kLoadGame, .width = kGrow});

                ui.button(
                    translator.text(MessageId::MenuWorldMap),
                    {.id = menuWidgets::kWorldMap, .width = kGrow});

                {
                    const auto row = ui.row({.width = kGrow});

                    ui.button(
                        translator.text(MessageId::MenuOptions),
                        {.id = menuWidgets::kOptions, .width = kGrow});

                    ui.button(
                        translator.text(MessageId::MenuQuit),
                        {.id = menuWidgets::kQuit, .width = kGrow});
                }
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

    void MainMenuScene::draw(
        IRenderer &renderer, const DrawList &picture) const
    {
        renderer.clear(kBackdrop);
        antwika::ui::paint(renderer, picture);
    }

}
