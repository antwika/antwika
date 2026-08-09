#pragma once

#include <span>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageName.hpp"
#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    template <typename IdType, const MessageTable<IdType> &Table>
    class CompiledMessages final
    {
    public:
        using Id = IdType;

        [[nodiscard]] static std::span<const MessageName<Id>>
            names() noexcept
        {
            return Table.names;
        }

        [[nodiscard]] static const Catalogue<Id> &catalogueFor(
            Locale locale) noexcept
        {
            return pickCatalogue(
                locale, englishCatalogue, swedishCatalogue);
        }

    private:
        static constexpr Catalogue<Id> englishCatalogue{
            Locale::English, Table.english};

        static constexpr Catalogue<Id> swedishCatalogue{
            Locale::Swedish, Table.swedish};
    };

}
