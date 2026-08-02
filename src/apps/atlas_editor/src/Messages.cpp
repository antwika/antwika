#include "antwika/atlas_editor/Messages.hpp"

#include <array>
#include <cstddef>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    namespace
    {

        using i18n::Catalogue;
        using i18n::CatalogueEntry;
        using i18n::Locale;
        using i18n::MessageName;

        constexpr std::array<MessageName<MessageId>, 16> kNames{{
            {MessageId::ToolPaint, "ToolPaint"},
            {MessageId::ToolErase, "ToolErase"},
            {MessageId::ToolPick, "ToolPick"},
            {MessageId::ResetView, "ResetView"},
            {MessageId::Grid, "Grid"},
            {MessageId::Load, "Load"},
            {MessageId::Save, "Save"},
            {MessageId::PixelUnknown, "PixelUnknown"},
            {MessageId::PixelAt, "PixelAt"},
            {MessageId::Slot, "Slot"},
            {MessageId::Unsaved, "Unsaved"},
            {MessageId::Saved, "Saved"},
            {MessageId::SaveFailed, "SaveFailed"},
            {MessageId::NothingToLoad, "NothingToLoad"},
            {MessageId::Loaded, "Loaded"},
            {MessageId::LoadFailed, "LoadFailed"},
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
                {MessageId::ToolPaint, "PAINT"},
                {MessageId::ToolErase, "ERASE"},
                {MessageId::ToolPick, "PICK"},
                {MessageId::ResetView, "fit"},
                {MessageId::Grid, "grid"},
                {MessageId::Load, "load"},
                {MessageId::Save, "save"},
                {MessageId::PixelUnknown, "px -,-"},
                {MessageId::PixelAt, "px {0},{1}"},
                {MessageId::Slot, "slot {0}"},
                {MessageId::Unsaved, "UNSAVED"},
                {MessageId::Saved, "saved {0}"},
                {MessageId::SaveFailed, "save failed: {0}"},
                {MessageId::NothingToLoad, "nothing to load"},
                {MessageId::Loaded, "loaded"},
                {MessageId::LoadFailed, "load failed: {0}"},
            }};

        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kSwedishEntries{{
                {MessageId::ToolPaint, "MÅLA"},
                {MessageId::ToolErase, "SUDDA"},
                {MessageId::ToolPick, "PLOCKA"},
                {MessageId::ResetView, "anpassa"},
                {MessageId::Grid, "rutnät"},
                {MessageId::Load, "läs in"},
                {MessageId::Save, "spara"},
                {MessageId::PixelUnknown, "px -,-"},
                {MessageId::PixelAt, "px {0},{1}"},
                {MessageId::Slot, "ruta {0}"},
                {MessageId::Unsaved, "OSPARAT"},
                {MessageId::Saved, "sparade {0}"},
                {MessageId::SaveFailed, "kunde inte spara: {0}"},
                {MessageId::NothingToLoad, "inget att läsa in"},
                {MessageId::Loaded, "inläst"},
                {MessageId::LoadFailed, "kunde inte läsa in: {0}"},
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
        switch (locale)
        {
        case Locale::English:
            return kEnglishCatalogue;
        case Locale::Swedish:
            return kSwedishCatalogue;
        }

        return catalogueFor(i18n::kDefaultLocale);
    }

} // namespace antwika::atlas_editor
