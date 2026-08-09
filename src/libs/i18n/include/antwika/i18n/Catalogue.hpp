#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "antwika/i18n/Locale.hpp"

namespace antwika::i18n
{

    template <typename Id>
    struct CatalogueEntry final
    {
        Id id{};

        std::string_view text;
    };

    template <typename Id>
    class Catalogue final
    {
    public:
        constexpr Catalogue(
            Locale locale,
            std::span<const CatalogueEntry<Id>> entries) noexcept
            : catalogueLocale{locale}, catalogueEntries{entries}
        {
        }

        [[nodiscard]] constexpr Locale locale() const noexcept
        {
            return catalogueLocale;
        }

        [[nodiscard]] constexpr std::span<const CatalogueEntry<Id>>
            entries() const noexcept
        {
            return catalogueEntries;
        }

        [[nodiscard]] constexpr std::optional<std::string_view> find(
            Id id) const noexcept
        {
            for (const CatalogueEntry<Id> &entry : catalogueEntries)
            {
                if (entry.id == id)
                {
                    return entry.text;
                }
            }

            return std::nullopt;
        }

    private:
        Locale catalogueLocale;
        std::span<const CatalogueEntry<Id>> catalogueEntries;
    };

    template <typename Id>
    [[nodiscard]] const Catalogue<Id> &pickCatalogue(
        Locale locale,
        const Catalogue<Id> &english,
        const Catalogue<Id> &swedish) noexcept
    {
        if (locale == Locale::Swedish)
        {
            return swedish;
        }

        return english;
    }

}
