#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageId.hpp"

namespace antwika::i18n
{

    /**
     * @brief One id and the text it stands for in one locale.
     */
    struct CatalogueEntry final
    {
        /**
         * @brief The id this entry answers for.
         */
        MessageId id{MessageId::MenuPlayGame};

        /**
         * @brief The text, which may contain `{0}`-style placeholders.
         */
        std::string_view text;
    };

    /**
     * @brief The strings one locale is written with.
     *
     * A non-owning view over entries that outlive it -- the compiled-in
     * catalogues are static, so nothing here allocates or reads a file.
     * Keeping the entries a span rather than a container is also what lets
     * a test build a deliberately incomplete catalogue from a local array
     * and watch the fallback rule work.
     */
    class Catalogue final
    {
    public:
        /**
         * @brief Build a catalogue over entries owned by the caller.
         * @param locale The language the entries are written in.
         * @param entries The entries, which must outlive this object.
         */
        constexpr Catalogue(
            Locale locale, std::span<const CatalogueEntry> entries) noexcept
            : catalogueLocale{locale}, catalogueEntries{entries}
        {
        }

        /**
         * @brief The language these entries are written in.
         * @return The locale.
         */
        [[nodiscard]] constexpr Locale locale() const noexcept
        {
            return catalogueLocale;
        }

        /**
         * @brief Every entry, in the order they were given.
         * @return The entries.
         */
        [[nodiscard]] constexpr std::span<const CatalogueEntry> entries()
            const noexcept
        {
            return catalogueEntries;
        }

        /**
         * @brief The text for one id, if this catalogue has it.
         * @param id The id to look up.
         * @return The text, or no value when this catalogue is silent
         *         about that id.
         */
        [[nodiscard]] std::optional<std::string_view> find(
            MessageId id) const noexcept;

    private:
        Locale catalogueLocale;
        std::span<const CatalogueEntry> catalogueEntries;
    };

    /**
     * @brief The compiled-in catalogue for a locale.
     * @param locale The locale wanted.
     * @return That locale's catalogue, or the default locale's for a value
     *         that is not one of the enumerators.
     */
    [[nodiscard]] const Catalogue &catalogueFor(Locale locale) noexcept;

} // namespace antwika::i18n
