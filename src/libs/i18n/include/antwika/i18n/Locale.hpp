#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/i18n/MessageId.hpp"

namespace antwika::i18n
{

    /**
     * @brief A language a catalogue is written in.
     *
     * Deliberately a closed set rather than an open language-tag string:
     * every locale this library answers for has a catalogue compiled into
     * it, so a tag it has never heard of is a parse failure at the edge
     * (see localeFromTag) rather than an empty catalogue in the middle.
     */
    enum class Locale : std::uint8_t
    {
        /**
         * @brief English, language tag `en`.
         */
        English,

        /**
         * @brief Swedish, language tag `sv`.
         */
        Swedish,
    };

    /**
     * @brief The locale every lookup falls back to.
     */
    inline constexpr Locale kDefaultLocale{Locale::English};

    /**
     * @brief Every locale, in declaration order.
     */
    inline constexpr std::array<Locale, 2> kAllLocales{
        Locale::English,
        Locale::Swedish,
    };

    /**
     * @brief The locale's language tag.
     * @param locale The locale to name.
     * @return The tag (`"en"`, `"sv"`), or `"?"` for a value that is not
     *         one of the enumerators.
     */
    [[nodiscard]] std::string_view tagOf(Locale locale) noexcept;

    /**
     * @brief The locale a language tag names.
     * @param tag The tag to resolve, matched exactly and case-sensitively.
     * @return The locale, or no value when no catalogue is compiled in for
     *         that tag.
     */
    [[nodiscard]] std::optional<Locale> localeFromTag(
        std::string_view tag) noexcept;

    /**
     * @brief The message id holding the locale's own name.
     *
     * The language names are messages like any other, so a language picker
     * reads them through the *active* translator: the list says
     * "English"/"Swedish" while English is on and "Engelska"/"Svenska"
     * while Swedish is.
     * @param locale The locale to name.
     * @return The id of that locale's name, or the default locale's for a
     *         value that is not one of the enumerators -- the same
     *         fall-back-to-default rule lookups follow.
     */
    [[nodiscard]] MessageId nameIdOf(Locale locale) noexcept;

} // namespace antwika::i18n
