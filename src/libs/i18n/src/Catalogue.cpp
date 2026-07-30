#include "antwika/i18n/Catalogue.hpp"

#include <array>
#include <optional>
#include <span>
#include <string_view>

#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageId.hpp"

namespace antwika::i18n
{

    namespace
    {

        // Both arrays list every id, in the same order.
        // CatalogueTest asserts they cover exactly kAllMessageIds.
        // That assertion is the point of keying by id.
        // A forgotten Swedish entry is a red build, not a wrong label.
        constexpr std::array<CatalogueEntry, kMessageCount> kEnglishEntries{{
            {MessageId::MenuPlayGame, "Play game"},
            {MessageId::MenuLoadReplay, "Load replay"},
            {MessageId::MenuSaveReplay, "Save replay"},
            {MessageId::MenuResumeGame, "Resume game"},
            {MessageId::MenuLanguage, "Language"},
            {MessageId::LanguageEnglish, "English"},
            {MessageId::LanguageSwedish, "Swedish"},
            {MessageId::ToolbarZoomIn, "zoom in"},
            {MessageId::ToolbarZoomOut, "zoom out"},
            {MessageId::ToolbarResetView, "reset view"},
            {MessageId::ToolbarZoomLevel, "zoom {0}"},
        }};

        constexpr std::array<CatalogueEntry, kMessageCount> kSwedishEntries{{
            {MessageId::MenuPlayGame, "Spela"},
            {MessageId::MenuLoadReplay, "Läs in repris"},
            {MessageId::MenuSaveReplay, "Spara repris"},
            {MessageId::MenuResumeGame, "Återuppta spel"},
            {MessageId::MenuLanguage, "Språk"},
            {MessageId::LanguageEnglish, "Engelska"},
            {MessageId::LanguageSwedish, "Svenska"},
            {MessageId::ToolbarZoomIn, "zooma in"},
            {MessageId::ToolbarZoomOut, "zooma ut"},
            {MessageId::ToolbarResetView, "återställ vy"},
            {MessageId::ToolbarZoomLevel, "zoom {0}"},
        }};

        constexpr Catalogue kEnglishCatalogue{
            Locale::English, kEnglishEntries};

        constexpr Catalogue kSwedishCatalogue{
            Locale::Swedish, kSwedishEntries};

    } // namespace

    std::optional<std::string_view> Catalogue::find(
        MessageId id) const noexcept
    {
        for (const CatalogueEntry &entry : catalogueEntries)
        {
            if (entry.id == id)
            {
                return entry.text;
            }
        }

        return std::nullopt;
    }

    const Catalogue &catalogueFor(Locale locale) noexcept
    {
        switch (locale)
        {
        case Locale::English:
            return kEnglishCatalogue;
        case Locale::Swedish:
            return kSwedishCatalogue;
        }

        return catalogueFor(kDefaultLocale);
    }

} // namespace antwika::i18n
