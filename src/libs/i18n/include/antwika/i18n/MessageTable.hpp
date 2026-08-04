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

    /**
     * @brief How many ids an enumeration declares, read off its Count.
     * @tparam Id An enumeration whose last enumerator is `Count`.
     *
     * Sizing a module's tables by this is what makes the count right by
     * construction.
     * A module used to write the length out as a literal and
     * static_assert it against `Count`; an array sized from the
     * enumeration cannot disagree with it, so an enumerator nobody
     * wrote a line for leaves a value-initialised hole rather than a
     * table one entry short.
     */
    template <typename Id>
    inline constexpr std::size_t messageCount =
        static_cast<std::size_t>(Id::Count);

    /**
     * @brief One module's names and both locales' text, in one object.
     * @tparam Id The module's message id enumeration.
     *
     * A module used to declare three arrays and two catalogues in its
     * `Messages.cpp` and define `names()` and `catalogueFor()` over
     * them, and the eight copies of that tail differed in nothing but
     * the strings.
     * The strings are the module's; the tail was not, and it is
     * CompiledMessages now.
     *
     * A table is the one thing a module still writes.
     * It is declared `extern const` in the module's `Messages.hpp`, so
     * that the alias beside it can name it as a template argument, and
     * defined `constexpr` in the module's `Messages.cpp`, where the
     * static_assert that nothing in it is missing can read it.
     */
    template <typename Id>
    struct MessageTable final
    {
        /**
         * @brief Every id there is, with the name it was declared under.
         */
        std::array<MessageName<Id>, messageCount<Id>> names;

        /**
         * @brief The English text for every id.
         */
        std::array<CatalogueEntry<Id>, messageCount<Id>> english;

        /**
         * @brief The Swedish text for every id.
         */
        std::array<CatalogueEntry<Id>, messageCount<Id>> swedish;
    };

    /**
     * @brief Whether every id in a table is named and worded twice over.
     * @tparam Id The module's message id enumeration.
     * @param table The table to check.
     * @return True when no name and no entry is missing or empty.
     *
     * This is the compile-time half of what MessageSetCompleteness
     * checks at run time, and a module static_asserts on it beside its
     * table.
     * A forgotten line is a value-initialised entry -- no name, or no
     * text -- so the hole a deleted Swedish string leaves is exactly
     * what this looks for, and it is a red build rather than an English
     * label in a Swedish window.
     *
     * It is deliberately silent about a repeated id, which takes a set
     * to see and is already the shared suite's job.
     */
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

} // namespace antwika::i18n
