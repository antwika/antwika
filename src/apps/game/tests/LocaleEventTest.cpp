#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

#include <antwika/i18n/Locale.hpp>

#include "antwika/game/LocaleEvent.hpp"
#include "antwika/game/OptionsFormatError.hpp"

using antwika::game::localeFromPayload;
using antwika::game::OptionsFormatError;
using antwika::game::setLocalePayload;
using antwika::i18n::kAllLocales;
using antwika::i18n::Locale;

// The tag rather than the enumerator's number.
// For the reason a key binding writes a key's name.
// A recording is read by builds this one has never met.
TEST(LocaleEventTest, ALanguageIsWrittenAsItsTag)
{
    const auto parsed =
        nlohmann::json::parse(setLocalePayload(Locale::Swedish));

    EXPECT_EQ(parsed.at("locale").get<std::string>(), "sv");
}

TEST(LocaleEventTest, EveryLanguageRoundTrips)
{
    for (const auto locale : kAllLocales)
    {
        EXPECT_EQ(localeFromPayload(setLocalePayload(locale)), locale);
    }
}

TEST(LocaleEventTest, APayloadThatIsNotJsonIsRefused)
{
    EXPECT_THROW(
        (void)localeFromPayload("not json at all"), OptionsFormatError);
}

TEST(LocaleEventTest, APayloadOfTheWrongShapeIsRefused)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"language":"sv"})"),
        OptionsFormatError);
}

TEST(LocaleEventTest, APayloadCarryingSomethingElseAsWellIsRefused)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"locale":"sv","extra":1})"),
        OptionsFormatError);
}

TEST(LocaleEventTest, ALocaleOfTheWrongTypeIsRefused)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"locale":7})"), OptionsFormatError);
}

// A build with no catalogue for it says so, rather than falling back.
// A language nobody could have picked is not one to guess at.
TEST(LocaleEventTest, ATagThisBuildHasNoCatalogueForIsRefused)
{
    EXPECT_THROW(
        (void)localeFromPayload(R"({"locale":"de"})"),
        OptionsFormatError);
}
