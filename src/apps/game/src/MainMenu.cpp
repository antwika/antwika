#include "antwika/game/MainMenu.hpp"

#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/game/MenuLabels.hpp"

namespace antwika::game
{

    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        // Which language is in force is state, so the selector says so.
        // Inferring it from the other buttons is what it saves.
        // A reader who cannot read those has nothing else to go on.
        [[nodiscard]] std::optional<ButtonState> selectionOf(
            MenuLanguage shown, MenuLanguage chosen) noexcept
        {
            return shown == chosen
                       ? std::optional<ButtonState>{ButtonState::Pressed}
                       : std::nullopt;
        }
    } // namespace

    Frame MainMenu::describe(
        Size canvas, Pointer pointer, const MenuState &state) const
    {
        const auto labels = labelsFor(state.language);

        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            // The whole canvas, deliberately: see MainMenu.hpp.
            const auto sheet = ui.panel(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center});

            ui.label(labels.title);

            for (const auto entry : entriesFor(state.gameBegun))
            {
                ui.button(
                    labelFor(labels, entry), {.id = widgetFor(entry)});
            }

            ui.label(labels.language);

            {
                const auto row =
                    ui.row({.width = kFit, .cross = Alignment::Center});

                for (const auto language : kMenuLanguages)
                {
                    ui.button(
                        labelFor(labels, language),
                        {.id = widgetForLanguage(language),
                         .state = selectionOf(language, state.language)});
                }
            }
        }

        return ui.finish();
    }

} // namespace antwika::game
