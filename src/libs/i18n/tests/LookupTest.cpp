#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Lookup.hpp>
#include <antwika/i18n/Translation.hpp>

#include "SampleMessages.hpp"

namespace
{

    using antwika::i18n::Catalogue;
    using antwika::i18n::CatalogueEntry;
    using antwika::i18n::format;
    using antwika::i18n::Locale;
    using antwika::i18n::lookup;
    using antwika::i18n::tests::kSampleEnglishCatalogue;
    using antwika::i18n::tests::kSampleSwedishCatalogue;
    using antwika::i18n::tests::SampleId;
    using antwika::i18n::tests::SampleMessages;
    using antwika::i18n::Translation;
    using antwika::i18n::TranslationOrigin;

    constexpr std::array<std::string_view, 2> kArgs{"2", "4"};

    TEST(LookupTest, Lookup_TakesTheActiveCatalogueWhenItHasTheId)
    {
        const Translation translation = lookup<SampleMessages>(
            SampleId::Play, kSampleSwedishCatalogue, kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "Spela");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(LookupTest, Lookup_FallsBackWhenTheActiveCatalogueIsSilent)
    {
        const Translation translation = lookup<SampleMessages>(
            SampleId::Language,
            kSampleSwedishCatalogue,
            kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "Language");
        EXPECT_EQ(translation.origin, TranslationOrigin::Fallback);
    }

    TEST(LookupTest, Lookup_ReportsAnIdNeitherCatalogueKnows)
    {
        const Translation translation = lookup<SampleMessages>(
            SampleId::Absent, kSampleSwedishCatalogue, kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "!Absent!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

    TEST(LookupTest, Lookup_ReportsAnIdThatIsNotAnEnumeratorAtAll)
    {
        const Translation translation = lookup<SampleMessages>(
            static_cast<SampleId>(9999),
            kSampleSwedishCatalogue,
            kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "!?!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

    TEST(LookupTest, Format_SubstitutesIntoTheActiveCatalogueText)
    {
        const Translation translation = format<SampleMessages>(
            SampleId::Level,
            kArgs,
            kSampleSwedishCatalogue,
            kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "zoom 2 av 4");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(LookupTest, Format_SubstitutesIntoAFallbackTextToo)
    {
        constexpr std::array<CatalogueEntry<SampleId>, 0> none{};
        const Catalogue<SampleId> empty{Locale::Swedish, none};

        const Translation translation = format<SampleMessages>(
            SampleId::Level, kArgs, empty, kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "zoom 2 of 4");
        EXPECT_EQ(translation.origin, TranslationOrigin::Fallback);
    }

    TEST(LookupTest, Format_LeavesAMissingTextAsItIs)
    {
        const Translation translation = format<SampleMessages>(
            SampleId::Absent,
            kArgs,
            kSampleSwedishCatalogue,
            kSampleEnglishCatalogue);

        EXPECT_EQ(translation.text, "!Absent!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

}
