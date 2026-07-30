#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/i18n/Translation.hpp>
#include <antwika/i18n/Translator.hpp>

namespace
{

    using antwika::i18n::kAllLocales;
    using antwika::i18n::kAllMessageIds;
    using antwika::i18n::Locale;
    using antwika::i18n::MessageId;
    using antwika::i18n::nameIdOf;
    using antwika::i18n::Translation;
    using antwika::i18n::TranslationOrigin;
    using antwika::i18n::Translator;

    TEST(TranslatorTest, Locale_ReportsWhatItWasBuiltWith)
    {
        EXPECT_EQ(Translator{Locale::Swedish}.locale(), Locale::Swedish);
    }

    TEST(TranslatorTest, SetLocale_ChangesWhichCatalogueAnswers)
    {
        Translator translator{Locale::English};

        EXPECT_EQ(translator.text(MessageId::MenuPlayGame), "Play game");

        translator.setLocale(Locale::Swedish);

        EXPECT_EQ(translator.locale(), Locale::Swedish);
        EXPECT_EQ(translator.text(MessageId::MenuPlayGame), "Spela");
    }

    TEST(TranslatorTest, Lookup_ResolvesEveryIdExactlyInEveryLocale)
    {
        for (const Locale locale : kAllLocales)
        {
            const Translator translator{locale};

            for (const MessageId id : kAllMessageIds)
            {
                const Translation translation = translator.lookup(id);

                EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
                EXPECT_FALSE(translation.text.empty());
            }
        }
    }

    TEST(TranslatorTest, Text_NamesEachLanguageInTheLanguageInUse)
    {
        const Translator english{Locale::English};
        const Translator swedish{Locale::Swedish};

        EXPECT_EQ(english.text(nameIdOf(Locale::English)), "English");
        EXPECT_EQ(english.text(nameIdOf(Locale::Swedish)), "Swedish");
        EXPECT_EQ(swedish.text(nameIdOf(Locale::English)), "Engelska");
        EXPECT_EQ(swedish.text(nameIdOf(Locale::Swedish)), "Svenska");
    }

    TEST(TranslatorTest, Format_SubstitutesTheZoomLevelIntoTheLabel)
    {
        const Translator translator{Locale::Swedish};
        constexpr std::array<std::string_view, 1> args{"3"};

        const Translation translation =
            translator.format(MessageId::ToolbarZoomLevel, args);

        EXPECT_EQ(translation.text, "zoom 3");
        EXPECT_EQ(translation.origin, TranslationOrigin::Exact);
    }

    TEST(TranslatorTest, Formatted_KeepsOnlyTheSubstitutedText)
    {
        const Translator translator{Locale::English};
        constexpr std::array<std::string_view, 1> args{"7"};

        EXPECT_EQ(
            translator.formatted(MessageId::ToolbarZoomLevel, args),
            "zoom 7");
    }

} // namespace
