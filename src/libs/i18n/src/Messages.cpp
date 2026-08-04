#include "antwika/i18n/Messages.hpp"

#include <array>
#include <cstddef>
#include <span>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/MessageName.hpp"

namespace antwika::i18n
{

    namespace
    {

        constexpr std::array<MessageName<MessageId>, 2> kNames{{
            {MessageId::LanguageEnglish, "LanguageEnglish"},
            {MessageId::LanguageSwedish, "LanguageSwedish"},
        }};

        static_assert(
            kNames.size() == static_cast<std::size_t>(MessageId::Count),
            "every MessageId must appear in kNames exactly once");

        // Both arrays list every id, in the same order.
        // MessagesTest asserts they cover exactly kNames.
        // That assertion is the point of keying by id.
        // A forgotten Swedish entry is a red build, not a wrong label.
        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kEnglishEntries{{
                {MessageId::LanguageEnglish, "English"},
                {MessageId::LanguageSwedish, "Swedish"},
            }};

        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kSwedishEntries{{
                {MessageId::LanguageEnglish, "Engelska"},
                {MessageId::LanguageSwedish, "Svenska"},
            }};

        constexpr Catalogue<MessageId> kEnglishCatalogue{
            Locale::English, kEnglishEntries};

        constexpr Catalogue<MessageId> kSwedishCatalogue{
            Locale::Swedish, kSwedishEntries};

    } // namespace

    std::span<const MessageName<MessageId>> Messages::names() noexcept
    {
        return kNames;
    }

    const Catalogue<MessageId> &Messages::catalogueFor(
        Locale locale) noexcept
    {
        return antwika::i18n::pickCatalogue(
            locale, kEnglishCatalogue, kSwedishCatalogue);
    }

} // namespace antwika::i18n
