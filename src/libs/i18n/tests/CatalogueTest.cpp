#include <gtest/gtest.h>

#include <array>
#include <optional>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>

#include "TestMessages.hpp"

namespace
{

    using antwika::i18n::Catalogue;
    using antwika::i18n::CatalogueEntry;
    using antwika::i18n::Locale;
    using antwika::i18n::tests::TestId;

    TEST(CatalogueTest, Locale_ReportsWhatTheCatalogueWasBuiltWith)
    {
        constexpr std::array<CatalogueEntry<TestId>, 1> entries{{
            {TestId::Play, "spela"},
        }};

        const Catalogue<TestId> catalogue{Locale::Swedish, entries};

        EXPECT_EQ(catalogue.locale(), Locale::Swedish);
        EXPECT_EQ(catalogue.entries().size(), 1U);
        EXPECT_EQ(catalogue.entries()[0].text, "spela");
    }

    TEST(CatalogueTest, Find_ReturnsTheTextForAnIdItCarries)
    {
        constexpr std::array<CatalogueEntry<TestId>, 2> entries{{
            {TestId::Play, "one"},
            {TestId::Language, "two"},
        }};

        const Catalogue<TestId> catalogue{Locale::English, entries};

        EXPECT_EQ(catalogue.find(TestId::Play), "one");
        EXPECT_EQ(catalogue.find(TestId::Language), "two");
    }

    TEST(CatalogueTest, Find_ReturnsNothingForAnIdItDoesNotCarry)
    {
        constexpr std::array<CatalogueEntry<TestId>, 1> entries{{
            {TestId::Play, "one"},
        }};

        const Catalogue<TestId> catalogue{Locale::English, entries};

        EXPECT_EQ(catalogue.find(TestId::Language), std::nullopt);
    }

} // namespace
