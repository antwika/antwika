#pragma once

#include <string>

#include <antwika/i18n/Locale.hpp>

namespace antwika::game
{

    using antwika::i18n::Locale;

    [[nodiscard]] std::string setLocalePayload(Locale locale);

    [[nodiscard]] Locale localeFromPayload(const std::string &payload);

}
