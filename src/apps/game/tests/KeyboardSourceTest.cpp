#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/KeyboardEvent.hpp"
#include "antwika/game/KeyboardSource.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::kDefaultKeyboardLayout;
using antwika::game::keyboardFromPayload;
using antwika::game::KeyboardLayout;
using antwika::game::KeyboardSource;
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

TEST(KeyboardSourceTest, EventsFor_AnnouncesAPickedBoardUpFront)
{
    ReplaySource inner{oneStop()};
    KeyboardSource source{inner, KeyboardLayout::English};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].name, antwika::game::events::kSetKeyboard);
    EXPECT_EQ(
        keyboardFromPayload(events[0].payload),
        KeyboardLayout::English);

    EXPECT_EQ(events[1].name, antwika::engine::events::kStop);
}

TEST(KeyboardSourceTest, EventsFor_ItIsAnnouncedOnceAndOnlyOnce)
{
    ReplaySource inner{oneStop()};
    KeyboardSource source{inner, KeyboardLayout::English};

    EXPECT_EQ(source.eventsFor(0).size(), 2U);
    EXPECT_TRUE(source.eventsFor(1).empty());
}

TEST(KeyboardSourceTest, EventsFor_TheShippedBoardIsNotNews)
{
    ReplaySource inner{oneStop()};
    KeyboardSource source{inner, kDefaultKeyboardLayout};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::engine::events::kStop);
}

TEST(KeyboardSourceTest, EventsFor_AReplayIsAPurePassThrough)
{
    ReplaySource inner{oneStop()};
    KeyboardSource source{inner, std::nullopt};

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, antwika::engine::events::kStop);
}
