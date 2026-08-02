#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Lookup.hpp>
#include <antwika/i18n/Translation.hpp>

#include "TestMessages.hpp"

namespace
{

    using antwika::i18n::Catalogue;
    using antwika::i18n::CatalogueEntry;
    using antwika::i18n::format;
    using antwika::i18n::Locale;
    using antwika::i18n::lookup;
    using antwika::i18n::tests::kTestEnglishCatalogue;
    using antwika::i18n::tests::kTestSwedishCatalogue;
    using antwika::i18n::tests::TestId;
    using antwika::i18n::tests::TestMessages;
    using antwika::i18n::Translation;
    using antwika::i18n::TranslationOrigin;

    // The Swedish catalogue here is deliberately incomplete.
    // The catalogues an application ships are not.
    // Only a silent catalogue can reach the fallback rule.
    constexpr std::array<std::string_view, 2> kArgs{"2", "4"};

    TEST(LookupTest, Lookup_TakesTheActiveCatalogueWhenItHasTheId)
    {
        const Translation translation = lookup<TestMessages>(
            TestId::Play, kTestSwedishCatalogue, kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "Spela");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(LookupTest, Lookup_FallsBackWhenTheActiveCatalogueIsSilent)
    {
        const Translation translation = lookup<TestMessages>(
            TestId::Language,
            kTestSwedishCatalogue,
            kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "Language");
        EXPECT_EQ(translation.origin, TranslationOrigin::Fallback);
    }

    TEST(LookupTest, Lookup_ReportsAnIdNeitherCatalogueKnows)
    {
        const Translation translation = lookup<TestMessages>(
            TestId::Absent, kTestSwedishCatalogue, kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "!Absent!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

    TEST(LookupTest, Lookup_ReportsAnIdThatIsNotAnEnumeratorAtAll)
    {
        const Translation translation = lookup<TestMessages>(
            static_cast<TestId>(9999),
            kTestSwedishCatalogue,
            kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "!?!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

    TEST(LookupTest, Format_SubstitutesIntoTheActiveCatalogueText)
    {
        const Translation translation = format<TestMessages>(
            TestId::Level,
            kArgs,
            kTestSwedishCatalogue,
            kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "zoom 2 av 4");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(LookupTest, Format_SubstitutesIntoAFallbackTextToo)
    {
        constexpr std::array<CatalogueEntry<TestId>, 0> none{};
        const Catalogue<TestId> empty{Locale::Swedish, none};

        const Translation translation = format<TestMessages>(
            TestId::Level, kArgs, empty, kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "zoom 2 of 4");
        EXPECT_EQ(translation.origin, TranslationOrigin::Fallback);
    }

    TEST(LookupTest, Format_LeavesAMissingTextAsItIs)
    {
        const Translation translation = format<TestMessages>(
            TestId::Absent,
            kArgs,
            kTestSwedishCatalogue,
            kTestEnglishCatalogue);

        EXPECT_EQ(translation.text, "!Absent!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

} // namespace
