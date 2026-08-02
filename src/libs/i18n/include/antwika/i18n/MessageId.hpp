#pragma once

#include <cstdint>

namespace antwika::i18n
{

    /**
     * @brief The only strings this library itself has to show.
     *
     * A library that enumerated its consumers' strings would be a library
     * naming its consumers, so every application declares its own
     * MessageId beside the code that shows it and owns its own
     * catalogues; see the MessageSet concept for what a module supplies
     * and how completeness is still checked.
     *
     * The names of the languages are the exception, and they are here
     * because they are the one set of strings that belongs to the
     * library rather than to anybody using it: `Locale` is a closed enum
     * this library declares, so the text for each of its values is this
     * library's to carry.
     * They are reached through nameIdOf(), so a language picker reads
     * them through the *active* translator -- "English"/"Swedish" while
     * English is on, "Engelska"/"Svenska" while Swedish is.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief The name of the English language.
         */
        LanguageEnglish,

        /**
         * @brief The name of the Swedish language.
         */
        LanguageSwedish,

        /**
         * @brief How many ids there are; not an id itself.
         *
         * Messages.cpp static_asserts its name table against this, which
         * is what makes an enumerator nobody listed a build failure
         * rather than a string that is silently in no catalogue.
         */
        Count,
    };

} // namespace antwika::i18n
