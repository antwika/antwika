#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string_view>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Lookup.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/i18n/Translation.hpp>

namespace
{

    using antwika::i18n::Catalogue;
    using antwika::i18n::CatalogueEntry;
    using antwika::i18n::format;
    using antwika::i18n::Locale;
    using antwika::i18n::lookup;
    using antwika::i18n::MessageId;
    using antwika::i18n::Translation;
    using antwika::i18n::TranslationOrigin;

    // Deliberately incomplete, which the compiled-in catalogues are not.
    // Only a catalogue a test builds itself can reach the fallback rule.
    constexpr std::array<CatalogueEntry, 2> kPartialEntries{{
        {MessageId::MenuPlayGame, "Spela"},
        {MessageId::ToolbarZoomLevel, "zoom {0} av {1}"},
    }};

    constexpr std::array<CatalogueEntry, 3> kCompleteEntries{{
        {MessageId::MenuPlayGame, "Play game"},
        {MessageId::MenuLanguage, "Language"},
        {MessageId::ToolbarZoomLevel, "zoom {0} of {1}"},
    }};

    const Catalogue &partial()
    {
        static const Catalogue catalogue{Locale::Swedish, kPartialEntries};

        return catalogue;
    }

    const Catalogue &complete()
    {
        static const Catalogue catalogue{Locale::English, kCompleteEntries};

        return catalogue;
    }

    constexpr std::array<std::string_view, 2> kArgs{"2", "4"};

    TEST(LookupTest, Lookup_TakesTheActiveCatalogueWhenItHasTheId)
    {
        const Translation translation =
            lookup(MessageId::MenuPlayGame, partial(), complete());

        EXPECT_EQ(translation.text, "Spela");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(LookupTest, Lookup_FallsBackWhenTheActiveCatalogueIsSilent)
    {
        const Translation translation =
            lookup(MessageId::MenuLanguage, partial(), complete());

        EXPECT_EQ(translation.text, "Language");
        EXPECT_EQ(translation.origin, TranslationOrigin::Fallback);
    }

    TEST(LookupTest, Lookup_ReportsAnIdNeitherCatalogueKnows)
    {
        const Translation translation =
            lookup(MessageId::MenuSaveReplay, partial(), complete());

        EXPECT_EQ(translation.text, "!MenuSaveReplay!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

    TEST(LookupTest, Lookup_ReportsAnIdThatIsNotAnEnumeratorAtAll)
    {
        const Translation translation =
            lookup(static_cast<MessageId>(9999), partial(), complete());

        EXPECT_EQ(translation.text, "!?!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

    TEST(LookupTest, Format_SubstitutesIntoTheActiveCatalogueText)
    {
        const Translation translation = format(
            MessageId::ToolbarZoomLevel, kArgs, partial(), complete());

        EXPECT_EQ(translation.text, "zoom 2 av 4");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(LookupTest, Format_SubstitutesIntoAFallbackTextToo)
    {
        constexpr std::array<CatalogueEntry, 0> none{};
        const Catalogue empty{Locale::Swedish, none};

        const Translation translation = format(
            MessageId::ToolbarZoomLevel, kArgs, empty, complete());

        EXPECT_EQ(translation.text, "zoom 2 of 4");
        EXPECT_EQ(translation.origin, TranslationOrigin::Fallback);
    }

    TEST(LookupTest, Format_LeavesAMissingTextAsItIs)
    {
        const Translation translation = format(
            MessageId::MenuSaveReplay, kArgs, partial(), complete());

        EXPECT_EQ(translation.text, "!MenuSaveReplay!");
        EXPECT_EQ(translation.origin, TranslationOrigin::Missing);
    }

} // namespace
