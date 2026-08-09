#pragma once

#include <cstdint>

#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace optionsWidgets
    {
        inline constexpr WidgetId kBack{201};

        inline constexpr WidgetId kFirstAction{210};

        [[nodiscard]] constexpr WidgetId actionWidget(
            Action action) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstAction)
                + actionIndex(action));
        }

        inline constexpr WidgetId kFirstLanguage{260};

        [[nodiscard]] constexpr WidgetId languageWidget(
            antwika::i18n::Locale locale) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstLanguage)
                + static_cast<std::uint64_t>(locale));
        }

        inline constexpr WidgetId kFirstKeyboard{280};

        [[nodiscard]] constexpr WidgetId keyboardWidget(
            KeyboardLayout layout) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstKeyboard)
                + keyboardLayoutIndex(layout));
        }
    }

    class OptionsScene final
    {
    public:
        OptionsScene(
            const Translator &translator,
            const LanguageTranslator &languages);

        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const OptionsState &state,
            antwika::i18n::Locale active) const;

    private:
        const Translator &translator;
        const LanguageTranslator &languages;
    };

}
