#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/i18n/Messages.hpp>
#include <antwika/i18n/Translation.hpp>
#include <antwika/i18n/Translator.hpp>

#include "TestMessages.hpp"

namespace
{

    using antwika::i18n::Locale;
    using antwika::i18n::MessageId;
    using antwika::i18n::Messages;
    using antwika::i18n::nameIdOf;
    using antwika::i18n::tests::TestId;
    using antwika::i18n::tests::TestMessages;
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
        Translator<TestMessages> translator{Locale::English};

        EXPECT_EQ(translator.text(TestId::Play), "Play game");

        translator.setLocale(Locale::Swedish);

        EXPECT_EQ(translator.locale(), Locale::Swedish);
        EXPECT_EQ(translator.text(TestId::Play), "Spela");
    }

    TEST(TranslatorTest, Lookup_ReportsWhichCatalogueAnswered)
    {
        const Translator<TestMessages> translator{Locale::Swedish};

        EXPECT_EQ(
            translator.lookup(TestId::Play).origin,
            TranslationOrigin::Exact);
        EXPECT_EQ(
            translator.lookup(TestId::Language).origin,
            TranslationOrigin::Fallback);
        EXPECT_EQ(
            translator.lookup(TestId::Absent).origin,
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
        const Translator<TestMessages> translator{Locale::Swedish};
        constexpr std::array<std::string_view, 2> args{"3", "9"};

        const Translation translation =
            translator.format(TestId::Level, args);

        EXPECT_EQ(translation.text, "zoom 3 av 9");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(TranslatorTest, Formatted_KeepsOnlyTheSubstitutedText)
    {
        const Translator<TestMessages> translator{Locale::English};
        constexpr std::array<std::string_view, 2> args{"7", "8"};

        EXPECT_EQ(
            translator.formatted(TestId::Level, args), "zoom 7 of 8");
    }

} // namespace
