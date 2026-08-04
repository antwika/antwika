#pragma once

#include <antwika/i18n/Locale.hpp>

#include "antwika/game/LocaleState.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game::tests
{

    /**
     * @brief The one translator every test in this application words
     * itself with.
     *
     * At kDefaultLocale, exactly as main() fixes it: a layout is a
     * function of the strings declared into it and a hit-test is a
     * function of the layout, so a test that laid a bar out in one
     * language and asserted a pixel from another would be asserting
     * something the application never does.
     *
     * Shared rather than made per fixture so that no test can quietly
     * pick a different language and still pass.
     */
    inline constexpr Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    /**
     * @brief The language names, at the same locale as kTranslator.
     *
     * The picker on the options screen reads its captions from this
     * rather than from kTranslator, because `Locale` is i18n's own enum
     * and the text for its values is i18n's -- see LocaleState.hpp.
     * Kept beside kTranslator so the two cannot drift apart.
     */
    inline constexpr LanguageTranslator kLanguages{
        antwika::i18n::kDefaultLocale};

} // namespace antwika::game::tests
