#pragma once

#include <cstdint>
#include <string>

namespace antwika::i18n
{

    /**
     * @brief Where a resolved string came from.
     *
     * This is what makes a total lookup honest: the caller always gets
     * something drawable, and can still tell a real translation from a
     * gap.
     */
    enum class TranslationOrigin : std::uint8_t
    {
        /**
         * @brief The active locale's own catalogue answered.
         */
        Exact,

        /**
         * @brief The active locale had no entry and the default locale's
         *        catalogue answered.
         */
        Fallback,

        /**
         * @brief Neither catalogue had an entry; the text is the id's own
         *        name in exclamation marks.
         */
        Missing,
    };

    /**
     * @brief One resolved string and where it came from.
     */
    struct Translation final
    {
        /**
         * @brief The text to show, never empty.
         */
        std::string text;

        /**
         * @brief Which catalogue, if any, produced the text.
         */
        TranslationOrigin origin{TranslationOrigin::Exact};
    };

} // namespace antwika::i18n
