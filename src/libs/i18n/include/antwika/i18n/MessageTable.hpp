#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageName.hpp"

namespace antwika::i18n
{

    template <typename Id>
    inline constexpr std::size_t kMessageCount =
        static_cast<std::size_t>(Id::Count);

    template <typename Id>
    struct MessageTable final
    {
        std::array<MessageName<Id>, kMessageCount<Id>> names;

        std::array<CatalogueEntry<Id>, kMessageCount<Id>> english;

        std::array<CatalogueEntry<Id>, kMessageCount<Id>> swedish;
    };

    template <typename Id>
    [[nodiscard]] constexpr bool isComplete(
        const MessageTable<Id> &table) noexcept
    {
        const Catalogue<Id> english{Locale::English, table.english};
        const Catalogue<Id> swedish{Locale::Swedish, table.swedish};

        for (const MessageName<Id> &named : table.names)
        {
            if (named.name.empty())
            {
                return false;
            }

            const std::optional<std::string_view> englishText =
                english.find(named.id);

            if (!englishText.has_value() || englishText->empty())
            {
                return false;
            }

            const std::optional<std::string_view> swedishText =
                swedish.find(named.id);

            if (!swedishText.has_value() || swedishText->empty())
            {
                return false;
            }
        }

        return true;
    }

}
