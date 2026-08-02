#include "antwika/game/MenuModalScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::i18n::MessageId;
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
        // Dark and translucent, so the city reads as still being there.
        // An opaque fill would say the same thing MainMenuScene says.
        // Which is that nothing is behind it, and here something is.
        constexpr Color kScrim{
            .red = 6, .green = 8, .blue = 12, .alpha = 190};

        // Wide enough that both items come out the same width.
        // The items grow into a card that does not.
        // MainMenuScene's reason, and its number.
        // So the two read as the same furniture.
        constexpr std::uint32_t kCardWidth = 300;
    } // namespace

    MenuModalScene::MenuModalScene(const Translator &translator)
        : translator(translator)
    {
    }

    Frame MenuModalScene::describe(Size canvas, Pointer pointer) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            // The scrim is the whole canvas, and it is filled.
            // That fill is what makes pointerOverUi true everywhere.
            // Which is how a press is kept off the city underneath.
            // See MenuModalScene.hpp -- it is not decoration.
            const auto screen = ui.panel(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center,
                 .background = kScrim});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(
                    {.width = fixedSize(kCardWidth), .height = kFit});

                ui.label(translator.text(MessageId::GameModalTitle));

                ui.button(
                    translator.text(MessageId::GameModalMainMenu),
                    {.id = modalWidgets::kMainMenu, .width = kGrow});

                ui.button(
                    translator.text(MessageId::GameModalResume),
                    {.id = modalWidgets::kResume, .width = kGrow});
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

} // namespace antwika::game
