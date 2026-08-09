#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

#include <antwika/i18n/Locale.hpp>

#include "antwika/game/LocaleEvent.hpp"
#include "antwika/game/OptionsFormatError.hpp"

using antwika::game::localeFromPayload;
using antwika::game::OptionsFormatError;
using antwika::game::setLocalePayload;
using antwika::i18n::kAllLocales;
using antwika::i18n::Locale;

TEST(LocaleEventTest, SetLocalePayload_WritesALanguageAsATag)
{
    const auto parsed =
        nlohmann::json::parse(setLocalePayload(Locale::Swedish));

    EXPECT_EQ(parsed.at("locale").get<std::string>(), "sv");
}

TEST(LocaleEventTest, LocaleFromPayload_EveryLanguageRoundTrips)
{
    for (const auto locale : kAllLocales)
    {
        EXPECT_EQ(localeFromPayload(setLocalePayload(locale)), locale);
    }
}

TEST(LocaleEventTest, LocaleFromPayload_APayloadThatIsNotJsonIsRefused)
{
    EXPECT_THROW(
        (void)localeFromPayload("not json at all"), OptionsFormatError);
}

TEST(LocaleEventTest, LocaleFromPayload_RefusesTheWrongShape)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"language":"sv"})"),
        OptionsFormatError);
}

TEST(LocaleEventTest, LocaleFromPayload_RefusesExtraContent)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"locale":"sv","extra":1})"),
        OptionsFormatError);
}

TEST(LocaleEventTest, LocaleFromPayload_RefusesAWrongTypedLocale)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"locale":7})"), OptionsFormatError);
}

TEST(LocaleEventTest, LocaleFromPayload_RefusesAnUnknownTag)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"locale":"de"})"),
        OptionsFormatError);
}
