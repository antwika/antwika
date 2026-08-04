#pragma once

#include <span>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageName.hpp"
#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    /**
     * @brief One compiled-in MessageTable, served as an i18n::MessageSet.
     * @tparam IdType The module's message id enumeration.
     * @tparam Table The module's table, which outlives every caller.
     *
     * Every module that shows text wrote the same two definitions over
     * its own arrays -- a `names()` returning the name table and a
     * `catalogueFor()` picking between two catalogues built beside it --
     * and the eight copies differed in nothing but the namespace around
     * them.
     * A module now writes its table and one alias:
     *
     * ```cpp
     * extern const i18n::MessageTable<MessageId> kMessageTable;
     *
     * using Messages = i18n::CompiledMessages<MessageId, kMessageTable>;
     * ```
     *
     * The table is a template argument rather than a constructor
     * argument because a MessageSet is a set of *static* functions:
     * i18n::Translator names the type and never holds an instance, which
     * is what keeps a translator to one Locale in size and lets a
     * catalogue be reached with no indirection through an object.
     *
     * Taking it by reference is what lets the definition stay in the
     * module's `Messages.cpp`.
     * A translation unit that only includes the header sees the
     * declaration and needs no more than the table's address, so
     * nothing but the module that owns the strings ever compiles them.
     */
    template <typename IdType, const MessageTable<IdType> &Table>
    class CompiledMessages final
    {
    public:
        /**
         * @brief The ids this set answers for.
         */
        using Id = IdType;

        /**
         * @brief Every id there is, with the name it was declared under.
         * @return The name table, which outlives every caller.
         */
        [[nodiscard]] static std::span<const MessageName<Id>>
            names() noexcept
        {
            return Table.names;
        }

        /**
         * @brief The compiled-in catalogue for a locale.
         * @param locale The locale wanted.
         * @return That locale's catalogue, or the default locale's for a
         *         value that is not one of the enumerators.
         */
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

} // namespace antwika::i18n
