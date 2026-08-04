#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "antwika/i18n/Locale.hpp"

namespace antwika::i18n
{

    /**
     * @brief One id and the text it stands for in one locale.
     */
    template <typename Id>
    struct CatalogueEntry final
    {
        /**
         * @brief The id this entry answers for.
         */
        Id id{};

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
     *
     * The id type is a template parameter because the catalogue is the
     * library's machinery and the ids are the calling module's: see the
     * MessageSet concept for what a module supplies and why the
     * completeness guarantee survives the split.
     */
    template <typename Id>
    class Catalogue final
    {
    public:
        /**
         * @brief Build a catalogue over entries owned by the caller.
         * @param locale The language the entries are written in.
         * @param entries The entries, which must outlive this object.
         */
        constexpr Catalogue(
            Locale locale,
            std::span<const CatalogueEntry<Id>> entries) noexcept
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
        [[nodiscard]] constexpr std::span<const CatalogueEntry<Id>>
            entries() const noexcept
        {
            return catalogueEntries;
        }

        /**
         * @brief The text for one id, if this catalogue has it.
         * @param id The id to look up.
         * @return The text, or no value when this catalogue is silent
         *         about that id.
         */
        [[nodiscard]] constexpr std::optional<std::string_view> find(
            Id id) const noexcept
        {
            for (const CatalogueEntry<Id> &entry : catalogueEntries)
            {
                if (entry.id == id)
                {
                    return entry.text;
                }
            }

            return std::nullopt;
        }

    private:
        Locale catalogueLocale;
        std::span<const CatalogueEntry<Id>> catalogueEntries;
    };

    /**
     * @brief Choose the catalogue a locale asks for.
     * @tparam Id The message id the catalogues are keyed by.
     * @param locale The locale to serve.
     * @param english The English catalogue.
     * @param swedish The Swedish catalogue.
     * @return Whichever the locale names.
     *
     * Every module that owns a MessageSet had a copy of this switch,
     * fallback line included -- nine of them, all identical but for
     * the two constants they named.
     * The fallback is what makes the lookup total: a locale this build
     * has no catalogue for is served the default's rather than
     * reaching an arm no enumerator names.
     */
    template <typename Id>
    [[nodiscard]] const Catalogue<Id> &pickCatalogue(
        Locale locale,
        const Catalogue<Id> &english,
        const Catalogue<Id> &swedish) noexcept
    {
        if (locale == Locale::Swedish)
        {
            return swedish;
        }

        return english;
    }

} // namespace antwika::i18n
