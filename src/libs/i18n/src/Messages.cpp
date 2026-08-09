#include "antwika/i18n/Messages.hpp"

#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    constexpr MessageTable<MessageId> kMessageTable{
        .names{{
            {MessageId::LanguageEnglish, "LanguageEnglish"},
            {MessageId::LanguageSwedish, "LanguageSwedish"},
        }},
        .english{{
            {MessageId::LanguageEnglish, "English"},
            {MessageId::LanguageSwedish, "Swedish"},
        }},
        .swedish{{
            {MessageId::LanguageEnglish, "Engelska"},
            {MessageId::LanguageSwedish, "Svenska"},
        }},
    };

    static_assert(
        isComplete(kMessageTable),
        "every MessageId needs a name and text in both locales");

}
