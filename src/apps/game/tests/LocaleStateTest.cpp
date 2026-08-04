#include <gtest/gtest.h>

#include <string>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/i18n/Locale.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/LocaleEvent.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MessageId.hpp"
#include "antwika/game/OptionsFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::LocaleState;
using antwika::game::MessageId;
using antwika::game::OptionsFormatError;
using antwika::game::setLocalePayload;
using antwika::i18n::kDefaultLocale;
using antwika::i18n::Locale;

namespace
{
    [[nodiscard]] TickEvent tickEvent()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] TickEvent setLocaleEvent(Locale locale)
    {
        return TickEvent{
            .tick = 0,
            .event =
                Event{
                    .name = antwika::game::events::kSetLocale,
                    .payload = setLocalePayload(locale)}};
    }
} // namespace

TEST(LocaleStateTest, ARunStartsInTheLanguageItWasConstructedWith)
{
    const LocaleState state{Locale::Swedish};

    EXPECT_EQ(state.locale(), Locale::Swedish);
    EXPECT_EQ(state.next(), Locale::Swedish);
    EXPECT_EQ(state.translator().locale(), Locale::Swedish);
}

TEST(LocaleStateTest, TheDefaultIsTheShippedLanguage)
{
    const LocaleState state;

    EXPECT_EQ(state.locale(), kDefaultLocale);
}

// The whole reason this is a sink rather than a setter.
// A press is resolved against the layout it was made on.
TEST(LocaleStateTest, ARequestIsStagedAndDoesNotLandWithinTheTick)
{
    LocaleState state;

    state.request(Locale::Swedish);

    EXPECT_EQ(state.locale(), kDefaultLocale);
    EXPECT_EQ(state.next(), Locale::Swedish);
    EXPECT_EQ(state.translator().locale(), kDefaultLocale);
}

TEST(LocaleStateTest, AStagedRequestLandsAtTheTickBoundary)
{
    LocaleState state;

    state.request(Locale::Swedish);
    state.handle(tickEvent());

    EXPECT_EQ(state.locale(), Locale::Swedish);
    EXPECT_EQ(state.translator().locale(), Locale::Swedish);
}

// Both translators move together, or neither does.
// A caption and its language's name cannot then disagree.
TEST(LocaleStateTest, TheLanguageNamesFollowTheCaptions)
{
    LocaleState state;

    EXPECT_EQ(state.languages().locale(), kDefaultLocale);

    state.request(Locale::Swedish);
    state.handle(tickEvent());

    EXPECT_EQ(state.languages().locale(), Locale::Swedish);
    EXPECT_EQ(state.languages().locale(), state.translator().locale());
}

TEST(LocaleStateTest, AnAnnouncedLanguageIsStagedRatherThanApplied)
{
    LocaleState state;

    state.handle(setLocaleEvent(Locale::Swedish));

    EXPECT_EQ(state.locale(), kDefaultLocale);
    EXPECT_EQ(state.next(), Locale::Swedish);

    state.handle(tickEvent());

    EXPECT_EQ(state.locale(), Locale::Swedish);
}

TEST(LocaleStateTest, AnAnnouncementNamingNoCatalogueIsRefused)
{
    LocaleState state;

    EXPECT_THROW(
        state.handle(
            TickEvent{
                .tick = 0,
                .event =
                    Event{
                        .name = antwika::game::events::kSetLocale,
                        .payload = R"({"locale":"de"})"}}),
        OptionsFormatError);
}

TEST(LocaleStateTest, AnythingElseIsIgnored)
{
    LocaleState state;

    state.request(Locale::Swedish);
    state.handle(
        TickEvent{
            .tick = 0,
            .event = Event{.name = "game.score_increment"}});

    EXPECT_EQ(state.locale(), kDefaultLocale);
    EXPECT_EQ(state.next(), Locale::Swedish);
}

// The translator is handed out const.
// Which is what keeps this the only place a language changes.
TEST(LocaleStateTest, TheTranslatorItHandsOutWordsTheActiveLanguage)
{
    LocaleState state;

    EXPECT_EQ(state.translator().text(MessageId::OptionsBack), "Back");

    state.request(Locale::Swedish);
    state.handle(tickEvent());

    EXPECT_EQ(state.translator().text(MessageId::OptionsBack), "Tillbaka");
}
