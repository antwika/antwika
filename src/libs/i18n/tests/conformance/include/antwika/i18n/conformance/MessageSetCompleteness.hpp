#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>
#include <antwika/i18n/MessageSet.hpp>
#include <antwika/i18n/Translation.hpp>
#include <antwika/i18n/Translator.hpp>

namespace antwika::i18n::conformance
{

    /**
     * @brief Every promise a MessageSet makes, as one test suite.
     *
     * This is the whole return on keying a catalogue by a symbolic id
     * rather than by the English text.
     * Keyed by source string, a locale is "complete" only relative to
     * whichever strings somebody remembered to extract, and a locale
     * missing an entry falls through to the key -- which *is* readable
     * English prose, so a missing translation and a finished one are the
     * same thing at runtime and nobody can see the gap.
     * Keyed by id the gap is a value, and this suite is what turns it
     * into a red build.
     *
     * **It is a typed suite so the guarantee can live in one place while
     * the strings live in many.**
     * The ids belong to the module that shows them; the check does not
     * belong to any of them, and copying it per module would be copying
     * the one assertion that has to be right.
     * A module instantiates this the way a backend instantiates
     * GfxBackendConformance: one call site, no shared file to collide
     * on.
     *
     * A Traits supplies the module's MessageSet as `Messages` and the
     * ids whose two texts genuinely read the same in both locales as
     * `sameInBothLocales()`.
     * That exemption list is deliberately explicit, because a forgotten
     * Swedish entry looks exactly like a word that is the same in both;
     * the only way to be excused is to be named, and an entry that has
     * since been translated is named wrongly and fails too.
     */
    template <typename Traits>
    class MessageSetCompleteness : public ::testing::Test
    {
    public:
        using Messages = typename Traits::Messages;
        using Id = typename Messages::Id;

        static_assert(
            MessageSet<Messages>,
            "a Traits::Messages must satisfy antwika::i18n::MessageSet");

        [[nodiscard]] static std::set<Id> idsOf(
            const Catalogue<Id> &catalogue)
        {
            std::set<Id> ids;

            for (const CatalogueEntry<Id> &entry : catalogue.entries())
            {
                ids.insert(entry.id);
            }

            return ids;
        }

        [[nodiscard]] static std::set<Id> declared()
        {
            std::set<Id> ids;

            for (const MessageName<Id> &named : Messages::names())
            {
                ids.insert(named.id);
            }

            return ids;
        }

        [[nodiscard]] static bool shared(Id id)
        {
            const std::span<const Id> exempt = Traits::sameInBothLocales();

            return std::ranges::find(exempt, id) != exempt.end();
        }
    };

    TYPED_TEST_SUITE_P(MessageSetCompleteness);

    // The list a catalogue is checked against must name each id once.
    // Otherwise every check below is weaker than it reads.
    TYPED_TEST_P(MessageSetCompleteness, Names_ListEveryIdExactlyOnce)
    {
        using Fixture = TestFixture;
        using Id = typename Fixture::Id;
        using Raw = std::underlying_type_t<Id>;

        const auto names = Fixture::Messages::names();
        std::set<std::string_view> spellings;

        for (const MessageName<Id> &named : names)
        {
            EXPECT_FALSE(named.name.empty());
            EXPECT_NE(named.name, "?");
            EXPECT_LT(
                static_cast<std::size_t>(static_cast<Raw>(named.id)),
                names.size())
                << named.name << " is outside the enumeration";

            spellings.insert(named.name);
        }

        EXPECT_EQ(Fixture::declared().size(), names.size());
        EXPECT_EQ(spellings.size(), names.size());
    }

    TYPED_TEST_P(
        MessageSetCompleteness, NameOf_ReportsQuestionMarkForAnUnknownId)
    {
        using Fixture = TestFixture;
        using Messages = typename Fixture::Messages;
        using Id = typename Fixture::Id;
        using Raw = std::underlying_type_t<Id>;

        const auto beyond =
            static_cast<Id>(static_cast<Raw>(Messages::names().size()));

        EXPECT_EQ(nameOf<Messages>(beyond), "?");
    }

    TYPED_TEST_P(
        MessageSetCompleteness,
        CatalogueFor_ReturnsTheCatalogueOfEachLocale)
    {
        using Messages = typename TestFixture::Messages;

        for (const Locale locale : kAllLocales)
        {
            EXPECT_EQ(Messages::catalogueFor(locale).locale(), locale);
        }
    }

    TYPED_TEST_P(
        MessageSetCompleteness,
        CatalogueFor_FallsBackToTheDefaultWhenUnknown)
    {
        using Messages = typename TestFixture::Messages;

        EXPECT_EQ(
            &Messages::catalogueFor(static_cast<Locale>(200)),
            &Messages::catalogueFor(kDefaultLocale));
    }

    // This is why the catalogues are keyed by a symbolic id at all.
    // With the English text as the key it could not be written.
    // A missing Swedish entry would simply be an English one.
    TYPED_TEST_P(
        MessageSetCompleteness,
        CatalogueFor_CoversExactlyTheSameIdSetInEveryLocale)
    {
        using Fixture = TestFixture;
        using Messages = typename Fixture::Messages;

        const auto expected = Fixture::declared();

        for (const Locale locale : kAllLocales)
        {
            const auto &catalogue = Messages::catalogueFor(locale);

            EXPECT_EQ(Fixture::idsOf(catalogue), expected)
                << "locale " << tagOf(locale) << " covers a different set";
            EXPECT_EQ(catalogue.entries().size(), expected.size())
                << "locale " << tagOf(locale) << " repeats an id";
        }
    }

    TYPED_TEST_P(
        MessageSetCompleteness,
        CatalogueFor_ResolvesEveryIdInEveryLocale)
    {
        using Messages = typename TestFixture::Messages;
        using Id = typename TestFixture::Id;

        for (const Locale locale : kAllLocales)
        {
            const auto &catalogue = Messages::catalogueFor(locale);

            for (const MessageName<Id> &named : Messages::names())
            {
                const std::optional<std::string_view> text =
                    catalogue.find(named.id);

                ASSERT_TRUE(text.has_value())
                    << "locale " << tagOf(locale) << " is missing "
                    << named.name;
                EXPECT_FALSE(text->empty()) << named.name;
            }
        }
    }

    TYPED_TEST_P(
        MessageSetCompleteness,
        Translator_ResolvesEveryIdExactlyInEveryLocale)
    {
        using Messages = typename TestFixture::Messages;
        using Id = typename TestFixture::Id;

        for (const Locale locale : kAllLocales)
        {
            const Translator<Messages> translator{locale};

            for (const MessageName<Id> &named : Messages::names())
            {
                const Translation translation =
                    translator.lookup(named.id);

                EXPECT_EQ(translation.origin, TranslationOrigin::Exact)
                    << named.name << " in " << tagOf(locale);
                EXPECT_FALSE(translation.text.empty()) << named.name;
            }
        }
    }

    TYPED_TEST_P(
        MessageSetCompleteness,
        EveryLocaleTranslatesWhatItDoesNotShare)
    {
        using Fixture = TestFixture;
        using Messages = typename Fixture::Messages;
        using Id = typename Fixture::Id;

        const auto &standard = Messages::catalogueFor(kDefaultLocale);

        for (const Locale locale : kAllLocales)
        {
            if (locale == kDefaultLocale)
            {
                continue;
            }

            const auto &catalogue = Messages::catalogueFor(locale);

            for (const MessageName<Id> &named : Messages::names())
            {
                if (Fixture::shared(named.id))
                {
                    continue;
                }

                EXPECT_NE(
                    catalogue.find(named.id), standard.find(named.id))
                    << "an untranslated entry would read the same: "
                    << named.name;
            }
        }
    }

    // An exemption that has since been translated is a stale exemption.
    // Left in place it would excuse the next entry that goes missing.
    TYPED_TEST_P(
        MessageSetCompleteness,
        SameInBothLocales_NamesOnlyIdsThatReallyAre)
    {
        using Messages = typename TestFixture::Messages;

        const auto &standard = Messages::catalogueFor(kDefaultLocale);

        for (const auto id : TypeParam::sameInBothLocales())
        {
            for (const Locale locale : kAllLocales)
            {
                EXPECT_EQ(
                    Messages::catalogueFor(locale).find(id),
                    standard.find(id))
                    << nameOf<Messages>(id) << " is translated after all";
            }
        }
    }

    REGISTER_TYPED_TEST_SUITE_P(
        MessageSetCompleteness,
        Names_ListEveryIdExactlyOnce,
        NameOf_ReportsQuestionMarkForAnUnknownId,
        CatalogueFor_ReturnsTheCatalogueOfEachLocale,
        CatalogueFor_FallsBackToTheDefaultWhenUnknown,
        CatalogueFor_CoversExactlyTheSameIdSetInEveryLocale,
        CatalogueFor_ResolvesEveryIdInEveryLocale,
        Translator_ResolvesEveryIdExactlyInEveryLocale,
        EveryLocaleTranslatesWhatItDoesNotShare,
        SameInBothLocales_NamesOnlyIdsThatReallyAre);

} // namespace antwika::i18n::conformance
