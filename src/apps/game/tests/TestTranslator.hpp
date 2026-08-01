#pragma once

#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>

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
    inline constexpr antwika::i18n::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

} // namespace antwika::game::tests
