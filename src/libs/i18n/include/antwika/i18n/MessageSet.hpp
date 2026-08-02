#pragma once

#include <concepts>
#include <span>
#include <string_view>
#include <type_traits>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageName.hpp"

namespace antwika::i18n
{

    /**
     * @brief Everything one module tells this library about its strings.
     *
     * A message id is a symbolic name for one translatable string, and a
     * catalogue is keyed by it rather than by the English text, so a
     * locale that is missing a string produces a lookup that *reports* a
     * miss.
     * Keying by the English string would make a missing translation fall
     * through to English prose embedded in the calling code, which is
     * indistinguishable from a translation that happens to read the same
     * in both languages.
     *
     * **A catalogue is complete only against a list of everything there
     * is.**
     * That is why names() is part of this concept and not an
     * afterthought: it is the list, and without one an id could never be
     * checked for.
     * The ids themselves live in the module that shows them -- a library
     * that enumerated its consumers' strings would be a library naming
     * its consumers -- and what makes that safe is that the list, the
     * catalogues and the check over them are all in the same module too.
     *
     * **The check is the shared typed test suite**
     * MessageSetCompleteness, which every module instantiates over its
     * own set.
     * It compares each locale's catalogue against names() and against
     * every other locale's, so adding an enumerator and forgetting the
     * Swedish text is a red build rather than an English label in a
     * Swedish window -- exactly as it was when one enumeration held
     * every string in the tree, but now one call site per module rather
     * than one file every module edits.
     * A module ties its table to its enumeration with a static_assert on
     * a trailing Count enumerator, which closes the one gap the central
     * list had: an enumerator nobody added to the list was invisible to
     * it.
     *
     * A message id is never persisted -- not in a save, not in a replay,
     * not in a high-score file -- so its numbering is free and a module
     * may add, reorder or remove one without a migration.
     */
    template <typename Messages>
    concept MessageSet =
        std::is_enum_v<typename Messages::Id>
        && requires(Locale locale) {
               {
                   Messages::names()
               } -> std::same_as<
                     std::span<const MessageName<typename Messages::Id>>>;
               {
                   Messages::catalogueFor(locale)
               } -> std::same_as<const Catalogue<typename Messages::Id> &>;
           };

    /**
     * @brief The id's own name, for diagnostics and for a total lookup.
     * @tparam Messages The message set the id belongs to.
     * @param id The id to name.
     * @return The enumerator's name, or `"?"` for a value that is not one
     *         of the enumerators.
     */
    template <MessageSet Messages>
    [[nodiscard]] std::string_view nameOf(
        typename Messages::Id id) noexcept
    {
        using Id = typename Messages::Id;

        for (const MessageName<Id> &named : Messages::names())
        {
            if (named.id == id)
            {
                return named.name;
            }
        }

        return "?";
    }

} // namespace antwika::i18n
