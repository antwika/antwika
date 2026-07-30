#pragma once

#include <span>
#include <string>
#include <string_view>

#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/Translation.hpp"

namespace antwika::i18n
{

    /**
     * @brief One locale's worth of lookups, as a plain value.
     *
     * It holds a Locale and nothing else -- the catalogues it reads are
     * compiled in and static, so a translator is copyable, cheap and safe
     * to store next to whatever owns the current language setting.
     * The fallback is always the default locale's catalogue; see lookup()
     * in Lookup.hpp for the rule it follows.
     */
    class Translator final
    {
    public:
        /**
         * @brief Make a translator for one locale.
         * @param locale The language to translate into.
         */
        explicit constexpr Translator(Locale locale) noexcept
            : activeLocale{locale}
        {
        }

        /**
         * @brief The language being translated into.
         * @return The locale.
         */
        [[nodiscard]] constexpr Locale locale() const noexcept
        {
            return activeLocale;
        }

        /**
         * @brief Switch to another language.
         * @param locale The language to translate into from now on.
         */
        constexpr void setLocale(Locale locale) noexcept
        {
            activeLocale = locale;
        }

        /**
         * @brief Resolve one id, reporting where the text came from.
         * @param id The id to resolve.
         * @return The text and its origin.
         */
        [[nodiscard]] Translation lookup(MessageId id) const;

        /**
         * @brief Resolve one id, keeping only the text.
         * @param id The id to resolve.
         * @return The text, which is never empty.
         */
        [[nodiscard]] std::string text(MessageId id) const;

        /**
         * @brief Resolve one id and substitute arguments into it.
         * @param id The id to resolve.
         * @param args The arguments for its `{0}`-style placeholders.
         * @return The substituted text and the pattern's origin.
         */
        [[nodiscard]] Translation format(
            MessageId id, std::span<const std::string_view> args) const;

        /**
         * @brief Resolve and substitute, keeping only the text.
         * @param id The id to resolve.
         * @param args The arguments for its `{0}`-style placeholders.
         * @return The substituted text, which is never empty.
         */
        [[nodiscard]] std::string formatted(
            MessageId id, std::span<const std::string_view> args) const;

    private:
        Locale activeLocale;
    };

} // namespace antwika::i18n
