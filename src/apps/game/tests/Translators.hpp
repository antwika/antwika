#pragma once

#include <antwika/i18n/Locale.hpp>

#include "antwika/game/LocaleState.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game::tests
{

    inline constexpr Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    inline constexpr LanguageTranslator kLanguages{
        antwika::i18n::kDefaultLocale};

}
