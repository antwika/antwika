#pragma once

#include <span>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/MessageName.hpp"

namespace antwika::i18n
{

    /**
     * @brief This library's own ids, as a MessageSet.
     *
     * The shape every module repeats: an id type, the list of every id
     * there is, and one catalogue per locale.
     * Here it carries only the language names, which is the whole of
     * what this library has to say in words of its own.
     */
    struct Messages final
    {
        /**
         * @brief The ids this set answers for.
         */
        using Id = MessageId;

        /**
         * @brief Every id there is, with the name it was declared under.
         * @return The name table, which outlives every caller.
         */
        [[nodiscard]] static std::span<const MessageName<MessageId>>
            names() noexcept;

        /**
         * @brief The compiled-in catalogue for a locale.
         * @param locale The locale wanted.
         * @return That locale's catalogue, or the default locale's for a
         *         value that is not one of the enumerators.
         */
        [[nodiscard]] static const Catalogue<MessageId> &catalogueFor(
            Locale locale) noexcept;
    };

} // namespace antwika::i18n
