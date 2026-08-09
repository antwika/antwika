#pragma once

#include <cstdint>
#include <string>

namespace antwika::i18n
{

    enum class TranslationOrigin : std::uint8_t
    {
        Exact,

        Fallback,

        Missing,
    };

    struct Translation final
    {
        std::string text;

        TranslationOrigin origin{TranslationOrigin::Exact};
    };

}
