#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/BindingSource.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/KeyBindings.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::Action;
using antwika::game::BindingSource;
using antwika::game::bindKeyFromPayload;
using antwika::game::BindOutcome;
using antwika::game::kDefaultBindings;
using antwika::game::KeyBinding;
using antwika::game::KeyBindings;
using antwika::input::Key;
using antwika::replay::ReplaySource;

namespace
{
    [[nodiscard]] KeyBindings rebound()
    {
        KeyBindings bindings;
        EXPECT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
        return bindings;
    }

    [[nodiscard]] std::vector<TickEvent> oneStop()
    {
        return {
            TickEvent{
                .tick = 0,
                .event =
                    Event{.name = antwika::engine::events::kStop}}};
    }
}

TEST(BindingSourceTest, EventsFor_AnnouncesAChangeUpFront)
{
    ReplaySource inner{oneStop()};
    BindingSource source{inner, rebound()};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].name, antwika::game::events::kBindKey);
    EXPECT_EQ(
        bindKeyFromPayload(events[0].payload),
        (KeyBinding{.action = Action::Pause, .key = Key::J}));

    EXPECT_EQ(events[1].name, antwika::engine::events::kStop);
}

TEST(BindingSourceTest, EventsFor_TheShippedLayoutIsNotNews)
{
    ReplaySource inner{oneStop()};
    BindingSource source{inner, kDefaultBindings};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::engine::events::kStop);
}

TEST(BindingSourceTest, EventsFor_TheAnnouncementIsMadeOnlyOnce)
{
    ReplaySource inner{oneStop()};
    BindingSource source{inner, rebound()};

    EXPECT_EQ(source.eventsFor(0).size(), 2U);
    EXPECT_TRUE(source.eventsFor(1).empty());
    EXPECT_TRUE(source.eventsFor(2).empty());
}

TEST(BindingSourceTest, EventsFor_AReplayAnnouncesNothingAtAll)
{
    ReplaySource inner{oneStop()};
    BindingSource source{inner, std::nullopt};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::engine::events::kStop);
}

TEST(BindingSourceTest, EventsFor_EveryChangedBindingIsAnnounced)
{
    KeyBindings bindings;
    ASSERT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
    ASSERT_EQ(bindings.bind(Action::ZoomIn, Key::K), BindOutcome::Bound);

    ReplaySource inner{{}};
    BindingSource source{inner, bindings};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(
        bindKeyFromPayload(events[0].payload),
        (KeyBinding{.action = Action::Pause, .key = Key::J}));
    EXPECT_EQ(
        bindKeyFromPayload(events[1].payload),
        (KeyBinding{.action = Action::ZoomIn, .key = Key::K}));
}
