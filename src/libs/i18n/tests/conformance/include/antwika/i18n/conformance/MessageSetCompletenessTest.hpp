#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <type_traits>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>
#include <antwika/i18n/MessageSet.hpp>
#include <antwika/i18n/Translation.hpp>
#include <antwika/i18n/Translator.hpp>

namespace antwika::i18n::conformance
{

    template <typename Traits>
    class MessageSetCompletenessTest : public ::testing::Test
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

    TYPED_TEST_SUITE_P(MessageSetCompletenessTest);

    TYPED_TEST_P(MessageSetCompletenessTest, Names_ListEveryIdExactlyOnce)
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
                << named.name;

            spellings.insert(named.name);
        }

        EXPECT_EQ(Fixture::declared().size(), names.size());
        EXPECT_EQ(spellings.size(), names.size());
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest, NameOf_ReportsQuestionMarkForAnUnknownId)
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
        MessageSetCompletenessTest,
        CatalogueFor_ReturnsTheCatalogueOfEachLocale)
    {
        using Messages = typename TestFixture::Messages;

        for (const Locale locale : kAllLocales)
        {
            EXPECT_EQ(Messages::catalogueFor(locale).locale(), locale);
        }
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest,
        CatalogueFor_FallsBackToTheDefaultWhenUnknown)
    {
        using Messages = typename TestFixture::Messages;

        EXPECT_EQ(
            &Messages::catalogueFor(static_cast<Locale>(200)),
            &Messages::catalogueFor(kDefaultLocale));
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest,
        CatalogueFor_CoversExactlyTheSameIdSetInEveryLocale)
    {
        using Fixture = TestFixture;
        using Messages = typename Fixture::Messages;

        const auto expected = Fixture::declared();

        for (const Locale locale : kAllLocales)
        {
            const auto &catalogue = Messages::catalogueFor(locale);

            EXPECT_EQ(Fixture::idsOf(catalogue), expected)
                << tagOf(locale);
            EXPECT_EQ(catalogue.entries().size(), expected.size())
                << tagOf(locale);
        }
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest,
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
                    << tagOf(locale) << ' ' << named.name;
                EXPECT_FALSE(text->empty()) << named.name;
            }
        }
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest,
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
                    << named.name << ' ' << tagOf(locale);
                EXPECT_FALSE(translation.text.empty()) << named.name;
            }
        }
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest,
        CatalogueFor_TranslatesWhatALocaleDoesNotShare)
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
                    << named.name;
            }
        }
    }

    TYPED_TEST_P(
        MessageSetCompletenessTest,
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
                    << nameOf<Messages>(id);
            }
        }
    }

    REGISTER_TYPED_TEST_SUITE_P(
        MessageSetCompletenessTest,
        Names_ListEveryIdExactlyOnce,
        NameOf_ReportsQuestionMarkForAnUnknownId,
        CatalogueFor_ReturnsTheCatalogueOfEachLocale,
        CatalogueFor_FallsBackToTheDefaultWhenUnknown,
        CatalogueFor_CoversExactlyTheSameIdSetInEveryLocale,
        CatalogueFor_ResolvesEveryIdInEveryLocale,
        Translator_ResolvesEveryIdExactlyInEveryLocale,
        CatalogueFor_TranslatesWhatALocaleDoesNotShare,
        SameInBothLocales_NamesOnlyIdsThatReallyAre);

}
