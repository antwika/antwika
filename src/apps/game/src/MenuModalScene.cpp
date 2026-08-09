#include "antwika/game/MenuModalScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
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
        constexpr Color kScrim{
            .red = 6, .green = 8, .blue = 12, .alpha = 190};

        constexpr std::uint32_t kCardWidth = 300;
    }

    MenuModalScene::MenuModalScene(const Translator &translator)
        : translator(translator)
    {
    }

    Frame MenuModalScene::describe(Size canvas, Pointer pointer) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            const auto screen = ui.panel(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center,
                 .background = kScrim});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(
                    {.width = fixedSize(kCardWidth), .height = kFit});

                ui.label(translator.text(MessageId::ModalTitle));

                ui.button(
                    translator.text(MessageId::ModalMainMenu),
                    {.id = modalWidgets::kMainMenu, .width = kGrow});

                ui.button(
                    translator.text(MessageId::ModalResume),
                    {.id = modalWidgets::kResume, .width = kGrow});
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

}
