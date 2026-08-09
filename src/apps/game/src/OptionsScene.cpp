#include "antwika/game/OptionsScene.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game
{

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
        constexpr std::uint32_t kCardWidth = 360;

        [[nodiscard]] MessageId noticeMessage(
            std::optional<BindOutcome> outcome) noexcept
        {
            if (!outcome.has_value())
            {
                return MessageId::OptionsHint;
            }

            if (*outcome == BindOutcome::Reserved)
            {
                return MessageId::OptionsReserved;
            }

            if (*outcome == BindOutcome::Taken)
            {
                return MessageId::OptionsTaken;
            }

            return MessageId::OptionsBound;
        }
    }

    OptionsScene::OptionsScene(
        const Translator &translator, const LanguageTranslator &languages)
        : translator(translator), languages(languages)
    {
    }

    Frame OptionsScene::describe(
        Size canvas,
        Pointer pointer,
        const OptionsState &state,
        antwika::i18n::Locale active) const
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

                ui.label(translator.text(MessageId::OptionsTitle));

                for (const auto action : kActions)
                {
                    std::string bound{
                        antwika::input::toString(
                            state.bindings().keyFor(action))};

                    if (state.awaiting() == action)
                    {
                        bound = translator.text(
                            MessageId::OptionsPress);
                    }

                    const auto name =
                        translator.text(actionLabel(action));
                    const std::array<std::string_view, 2> parts{
                        name, bound};

                    ui.button(
                        translator.formatted(
                            MessageId::OptionsRow, parts),
                        {.id = optionsWidgets::actionWidget(action),
                         .width = kGrow});
                }

                ui.label(
                    translator.text(noticeMessage(state.notice())),
                    ui.theme().muted);

                ui.label(translator.text(MessageId::OptionsLanguage));

                for (const auto locale : antwika::i18n::kAllLocales)
                {
                    auto name =
                        languages.text(antwika::i18n::nameIdOf(locale));

                    if (locale == active)
                    {
                        name = translator.formatted(
                            MessageId::OptionsLanguageActive,
                            std::array<std::string_view, 1>{name});
                    }

                    ui.button(
                        name,
                        {.id = optionsWidgets::languageWidget(locale),
                         .width = kGrow});
                }

                {
                    const auto row = ui.row({.width = kGrow});

                    ui.label(
                        translator.text(MessageId::OptionsKeyboard));

                    for (const auto layout : kKeyboardLayouts)
                    {
                        auto name = translator.text(
                            keyboardLayoutLabel(layout));

                        if (layout == state.keyboard())
                        {
                            name = translator.formatted(
                                MessageId::OptionsLanguageActive,
                                std::array<std::string_view, 1>{name});
                        }

                        ui.button(
                            name,
                            {.id = optionsWidgets::keyboardWidget(
                                 layout),
                             .width = kGrow});
                    }
                }

                ui.button(
                    translator.text(MessageId::OptionsBack),
                    {.id = optionsWidgets::kBack, .width = kGrow});
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

}
