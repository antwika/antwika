#include "antwika/tower_defence/Messages.hpp"

#include <array>
#include <cstddef>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

#include "antwika/tower_defence/MessageId.hpp"

namespace antwika::tower_defence
{

    namespace
    {

        using i18n::Catalogue;
        using i18n::CatalogueEntry;
        using i18n::Locale;
        using i18n::MessageName;

        constexpr std::array<MessageName<MessageId>, 7> kNames{{
            {MessageId::Level, "Level"},
            {MessageId::Wave, "Wave"},
            {MessageId::Lives, "Lives"},
            {MessageId::Score, "Score"},
            {MessageId::Best, "Best"},
            {MessageId::Cleared, "Cleared"},
            {MessageId::Overrun, "Overrun"},
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
                {MessageId::Level, "lvl {0}/{1}"},
                {MessageId::Wave, "wave {0}/{1}"},
                {MessageId::Lives, "lives {0}"},
                {MessageId::Score, "score {0}"},
                {MessageId::Best, "best {0}"},
                {MessageId::Cleared, "CLEARED"},
                {MessageId::Overrun, "OVERRUN"},
            }};

        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kSwedishEntries{{
                {MessageId::Level, "nivå {0}/{1}"},
                {MessageId::Wave, "våg {0}/{1}"},
                {MessageId::Lives, "liv {0}"},
                {MessageId::Score, "poäng {0}"},
                {MessageId::Best, "bäst {0}"},
                {MessageId::Cleared, "KLARAT"},
                {MessageId::Overrun, "ÖVERMANNAD"},
            }};

        constexpr Catalogue<MessageId> kEnglishCatalogue{
            Locale::English, kEnglishEntries};

        constexpr Catalogue<MessageId> kSwedishCatalogue{
            Locale::Swedish, kSwedishEntries};

    } // namespace

    std::span<const i18n::MessageName<MessageId>>
        Messages::names() noexcept
    {
        return kNames;
    }

    const i18n::Catalogue<MessageId> &Messages::catalogueFor(
        i18n::Locale locale) noexcept
    {
        return antwika::i18n::pickCatalogue(
            locale, kEnglishCatalogue, kSwedishCatalogue);
    }

} // namespace antwika::tower_defence
