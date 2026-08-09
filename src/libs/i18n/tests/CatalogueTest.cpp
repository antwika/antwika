#include <gtest/gtest.h>

#include <array>
#include <optional>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>

#include "SampleMessages.hpp"

namespace
{

    using antwika::i18n::Catalogue;
    using antwika::i18n::CatalogueEntry;
    using antwika::i18n::Locale;
    using antwika::i18n::tests::SampleId;

    TEST(CatalogueTest, Locale_ReportsWhatTheCatalogueWasBuiltWith)
    {
        constexpr std::array<CatalogueEntry<SampleId>, 1> entries{{
            {SampleId::Play, "spela"},
        }};

        const Catalogue<SampleId> catalogue{Locale::Swedish, entries};

        EXPECT_EQ(catalogue.locale(), Locale::Swedish);
        EXPECT_EQ(catalogue.entries().size(), 1U);
        EXPECT_EQ(catalogue.entries()[0].text, "spela");
    }

    TEST(CatalogueTest, Find_ReturnsTheTextForAnIdItCarries)
    {
        constexpr std::array<CatalogueEntry<SampleId>, 2> entries{{
            {SampleId::Play, "one"},
            {SampleId::Language, "two"},
        }};

        const Catalogue<SampleId> catalogue{Locale::English, entries};

        EXPECT_EQ(catalogue.find(SampleId::Play), "one");
        EXPECT_EQ(catalogue.find(SampleId::Language), "two");
    }

    TEST(CatalogueTest, Find_ReturnsNothingForAnIdItDoesNotCarry)
    {
        constexpr std::array<CatalogueEntry<SampleId>, 1> entries{{
            {SampleId::Play, "one"},
        }};

        const Catalogue<SampleId> catalogue{Locale::English, entries};

        EXPECT_EQ(catalogue.find(SampleId::Language), std::nullopt);
    }

}
