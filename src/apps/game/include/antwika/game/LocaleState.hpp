#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Messages.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::i18n::Locale;

    using LanguageTranslator =
        antwika::i18n::Translator<antwika::i18n::Messages>;

    class LocaleState final : public ITickEventSink
    {
    public:
        explicit LocaleState(
            Locale initial = antwika::i18n::kDefaultLocale) noexcept
            : active(initial), languageNames(initial), staged(initial)
        {
        }

        LocaleState(const LocaleState &) = delete;
        LocaleState(LocaleState &&) = delete;

        LocaleState &operator=(const LocaleState &) = delete;
        LocaleState &operator=(LocaleState &&) = delete;

        [[nodiscard]] const Translator &translator() const noexcept
        {
            return active;
        }

        [[nodiscard]] const LanguageTranslator &languages() const noexcept
        {
            return languageNames;
        }

        [[nodiscard]] Locale locale() const noexcept
        {
            return active.locale();
        }

        [[nodiscard]] Locale next() const noexcept
        {
            return staged;
        }

        void request(Locale locale) noexcept
        {
            staged = locale;
        }

        void handle(const TickEvent &event) override;

    private:
        Translator active;
        LanguageTranslator languageNames;
        Locale staged;
    };

}
