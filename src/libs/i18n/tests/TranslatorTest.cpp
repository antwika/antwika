#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/i18n/Messages.hpp>
#include <antwika/i18n/Translation.hpp>
#include <antwika/i18n/Translator.hpp>

#include "SampleMessages.hpp"

namespace
{

    using antwika::i18n::Locale;
    using antwika::i18n::MessageId;
    using antwika::i18n::Messages;
    using antwika::i18n::nameIdOf;
    using antwika::i18n::tests::SampleId;
    using antwika::i18n::tests::SampleMessages;
    using antwika::i18n::Translation;
    using antwika::i18n::TranslationOrigin;
    using antwika::i18n::Translator;

    TEST(TranslatorTest, Locale_ReportsWhatItWasBuiltWith)
    {
        EXPECT_EQ(
            Translator<Messages>{Locale::Swedish}.locale(),
            Locale::Swedish);
    }

    TEST(TranslatorTest, SetLocale_ChangesWhichCatalogueAnswers)
    {
        Translator<SampleMessages> translator{Locale::English};

        EXPECT_EQ(translator.text(SampleId::Play), "Play game");

        translator.setLocale(Locale::Swedish);

        EXPECT_EQ(translator.locale(), Locale::Swedish);
        EXPECT_EQ(translator.text(SampleId::Play), "Spela");
    }

    TEST(TranslatorTest, Lookup_ReportsWhichCatalogueAnswered)
    {
        const Translator<SampleMessages> translator{Locale::Swedish};

        EXPECT_EQ(
            translator.lookup(SampleId::Play).origin,
            TranslationOrigin::Exact);
        EXPECT_EQ(
            translator.lookup(SampleId::Language).origin,
            TranslationOrigin::Fallback);
        EXPECT_EQ(
            translator.lookup(SampleId::Absent).origin,
            TranslationOrigin::Missing);
    }

    TEST(TranslatorTest, Text_NamesEachLanguageInTheLanguageInUse)
    {
        const Translator<Messages> english{Locale::English};
        const Translator<Messages> swedish{Locale::Swedish};

        EXPECT_EQ(english.text(nameIdOf(Locale::English)), "English");
        EXPECT_EQ(english.text(nameIdOf(Locale::Swedish)), "Swedish");
        EXPECT_EQ(swedish.text(nameIdOf(Locale::English)), "Engelska");
        EXPECT_EQ(swedish.text(nameIdOf(Locale::Swedish)), "Svenska");
    }

    TEST(TranslatorTest, Text_KeepsTheLanguageNamesTheLibraryShips)
    {
        const Translator<Messages> english{Locale::English};

        EXPECT_EQ(english.text(MessageId::LanguageEnglish), "English");
        EXPECT_EQ(english.text(MessageId::LanguageSwedish), "Swedish");
    }

    TEST(TranslatorTest, Format_SubstitutesTheZoomLevelIntoTheLabel)
    {
        const Translator<SampleMessages> translator{Locale::Swedish};
        constexpr std::array<std::string_view, 2> args{"3", "9"};

        const Translation translation =
            translator.format(SampleId::Level, args);

        EXPECT_EQ(translation.text, "zoom 3 av 9");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(TranslatorTest, Formatted_KeepsOnlyTheSubstitutedText)
    {
        const Translator<SampleMessages> translator{Locale::English};
        constexpr std::array<std::string_view, 2> args{"7", "8"};

        EXPECT_EQ(
            translator.formatted(SampleId::Level, args), "zoom 7 of 8");
    }

}
