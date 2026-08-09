#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/LocaleEvent.hpp"
#include "antwika/game/LocaleSource.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::localeFromPayload;
using antwika::game::LocaleSource;
using antwika::i18n::kDefaultLocale;
using antwika::i18n::Locale;
using antwika::replay::ReplaySource;

namespace
{
    [[nodiscard]] std::vector<TickEvent> oneStop()
    {
        return {
            TickEvent{
                .tick = 0,
                .event =
                    Event{.name = antwika::engine::events::kStop}}};
    }
}

TEST(LocaleSourceTest, EventsFor_AnnouncesALanguageUpFront)
{
    ReplaySource inner{oneStop()};
    LocaleSource source{inner, Locale::Swedish};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].name, antwika::game::events::kSetLocale);
    EXPECT_EQ(localeFromPayload(events[0].payload), Locale::Swedish);

    EXPECT_EQ(events[1].name, antwika::engine::events::kStop);
}

TEST(LocaleSourceTest, EventsFor_ItIsAnnouncedOnceAndOnlyOnce)
{
    ReplaySource inner{oneStop()};
    LocaleSource source{inner, Locale::Swedish};

    EXPECT_EQ(source.eventsFor(0).size(), 2U);
    EXPECT_TRUE(source.eventsFor(1).empty());
}

TEST(LocaleSourceTest, EventsFor_TheShippedLanguageIsNotNews)
{
    ReplaySource inner{oneStop()};
    LocaleSource source{inner, kDefaultLocale};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::engine::events::kStop);
}

TEST(LocaleSourceTest, EventsFor_AReplayIsAPurePassThrough)
{
    ReplaySource inner{oneStop()};
    LocaleSource source{inner, std::nullopt};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::engine::events::kStop);
}

TEST(LocaleSourceTest, EventsFor_CarriesItOnAnEmptyTick)
{
    ReplaySource inner{{}};
    LocaleSource source{inner, Locale::Swedish};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::game::events::kSetLocale);
}
