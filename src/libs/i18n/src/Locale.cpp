#include "antwika/i18n/Locale.hpp"

#include <optional>
#include <string_view>

#include "antwika/i18n/MessageId.hpp"

namespace antwika::i18n
{

    std::string_view tagOf(Locale locale) noexcept
    {
        switch (locale)
        {
        case Locale::English:
            return "en";
        case Locale::Swedish:
            return "sv";
        }

        return "?";
    }

    std::optional<Locale> localeFromTag(std::string_view tag) noexcept
    {
        for (const Locale locale : kAllLocales)
        {
            if (tagOf(locale) == tag)
            {
                return locale;
            }
        }

        return std::nullopt;
    }

    MessageId nameIdOf(Locale locale) noexcept
    {
        switch (locale)
        {
        case Locale::English:
            return MessageId::LanguageEnglish;
        case Locale::Swedish:
            return MessageId::LanguageSwedish;
        }

        return MessageId::LanguageEnglish;
    }

}
