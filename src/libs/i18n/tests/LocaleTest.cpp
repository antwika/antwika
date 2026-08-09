#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <set>
#include <string_view>

#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>

namespace
{

    using antwika::i18n::kAllLocales;
    using antwika::i18n::kDefaultLocale;
    using antwika::i18n::Locale;
    using antwika::i18n::localeFromTag;
    using antwika::i18n::MessageId;
    using antwika::i18n::nameIdOf;
    using antwika::i18n::tagOf;

    TEST(LocaleTest, AllLocales_ContainsTheDefaultLocale)
    {
        EXPECT_EQ(kDefaultLocale, Locale::English);
        EXPECT_NE(
            std::ranges::find(kAllLocales, kDefaultLocale),
            kAllLocales.end());
    }

    TEST(LocaleTest, TagOf_GivesEveryLocaleItsOwnDistinctTag)
    {
        std::set<std::string_view> tags;

        for (const Locale locale : kAllLocales)
        {
            tags.insert(tagOf(locale));
        }

        EXPECT_EQ(tags.size(), kAllLocales.size());
        EXPECT_EQ(tagOf(Locale::English), "en");
        EXPECT_EQ(tagOf(Locale::Swedish), "sv");
    }

    TEST(LocaleTest, TagOf_ReportsQuestionMarkForUnknownLocale)
    {
        EXPECT_EQ(tagOf(static_cast<Locale>(200)), "?");
    }

    TEST(LocaleTest, LocaleFromTag_ResolvesEveryLocalesOwnTag)
    {
        ASSERT_FALSE(kAllLocales.empty());

        for (const Locale locale : kAllLocales)
        {
            EXPECT_EQ(localeFromTag(tagOf(locale)), locale);
        }
    }

    TEST(LocaleTest, LocaleFromTag_ReportsNothingForAnUnknownTag)
    {
        EXPECT_EQ(localeFromTag("de"), std::nullopt);
        EXPECT_EQ(localeFromTag(""), std::nullopt);
        EXPECT_EQ(localeFromTag("EN"), std::nullopt);
    }

    TEST(LocaleTest, NameIdOf_NamesEveryLocale)
    {
        EXPECT_EQ(nameIdOf(Locale::English), MessageId::LanguageEnglish);
        EXPECT_EQ(nameIdOf(Locale::Swedish), MessageId::LanguageSwedish);
    }

    TEST(LocaleTest, NameIdOf_FallsBackToTheDefaultLocaleWhenUnknown)
    {
        EXPECT_EQ(
            nameIdOf(static_cast<Locale>(200)),
            MessageId::LanguageEnglish);
        EXPECT_EQ(
            nameIdOf(static_cast<Locale>(200)), nameIdOf(kDefaultLocale));
    }

}
