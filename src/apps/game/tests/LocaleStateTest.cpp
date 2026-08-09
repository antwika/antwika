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
}

TEST(LocaleStateTest, Locale_StartsAsConstructed)
{
    const LocaleState state{Locale::Swedish};

    EXPECT_EQ(state.locale(), Locale::Swedish);
    EXPECT_EQ(state.next(), Locale::Swedish);
    EXPECT_EQ(state.translator().locale(), Locale::Swedish);
}

TEST(LocaleStateTest, Locale_DefaultsToTheShippedLanguage)
{
    const LocaleState state;

    EXPECT_EQ(state.locale(), kDefaultLocale);
}

TEST(LocaleStateTest, Request_StagesWithoutLandingInTheTick)
{
    LocaleState state;

    state.request(Locale::Swedish);

    EXPECT_EQ(state.locale(), kDefaultLocale);
    EXPECT_EQ(state.next(), Locale::Swedish);
    EXPECT_EQ(state.translator().locale(), kDefaultLocale);
}

TEST(LocaleStateTest, Handle_LandsAStagedRequestOnATick)
{
    LocaleState state;

    state.request(Locale::Swedish);
    state.handle(tickEvent());

    EXPECT_EQ(state.locale(), Locale::Swedish);
    EXPECT_EQ(state.translator().locale(), Locale::Swedish);
}

TEST(LocaleStateTest, Locale_NamesFollowTheCaptions)
{
    LocaleState state;

    EXPECT_EQ(state.languages().locale(), kDefaultLocale);

    state.request(Locale::Swedish);
    state.handle(tickEvent());

    EXPECT_EQ(state.languages().locale(), Locale::Swedish);
    EXPECT_EQ(state.languages().locale(), state.translator().locale());
}

TEST(LocaleStateTest, Handle_StagesAnAnnouncedLanguage)
{
    LocaleState state;

    state.handle(setLocaleEvent(Locale::Swedish));

    EXPECT_EQ(state.locale(), kDefaultLocale);
    EXPECT_EQ(state.next(), Locale::Swedish);

    state.handle(tickEvent());

    EXPECT_EQ(state.locale(), Locale::Swedish);
}

TEST(LocaleStateTest, Handle_RefusesAnUnknownCatalogue)
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

TEST(LocaleStateTest, Handle_AnythingElseIsIgnored)
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

TEST(LocaleStateTest, Translator_WordsTheActiveLanguage)
{
    LocaleState state;

    EXPECT_EQ(state.translator().text(MessageId::OptionsBack), "Back");

    state.request(Locale::Swedish);
    state.handle(tickEvent());

    EXPECT_EQ(state.translator().text(MessageId::OptionsBack), "Tillbaka");
}
