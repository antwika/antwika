#include <gtest/gtest.h>

#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/StopOnKeySource.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::StopOnKeySource;
using antwika::replay::ReplaySource;

namespace
{
    constexpr auto kStop = antwika::engine::events::kStop;

    [[nodiscard]] TickEvent keyDown(Key key, bool repeat = false)
    {
        const InputEventCodec codec;
        return TickEvent{
            .tick = 0,
            .event = codec.encode(
                KeyPressed{.key = key, .repeat = repeat})};
    }

    [[nodiscard]] bool holdsStop(const std::vector<Event> &events)
    {
        for (const auto &event : events)
        {
            if (event.name == kStop)
            {
                return true;
            }
        }

        return false;
    }
}

TEST(StopOnKeySourceTest, EventsFor_AppendsAStopWhenTheChosenKeyGoesDown)
{
    ReplaySource innerSource({keyDown(Key::Escape)});
    const InputEventCodec codec;
    StopOnKeySource source(innerSource, codec, Key::Escape);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events.back().name, kStop);
}

TEST(StopOnKeySourceTest, EventsFor_LeavesADifferentKeyAlone)
{
    ReplaySource innerSource({keyDown(Key::W)});
    const InputEventCodec codec;
    StopOnKeySource source(innerSource, codec, Key::Escape);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_FALSE(holdsStop(events));
}

TEST(StopOnKeySourceTest, EventsFor_IgnoresARepeatOfTheChosenKey)
{
    ReplaySource innerSource({keyDown(Key::Escape, true)});
    const InputEventCodec codec;
    StopOnKeySource source(innerSource, codec, Key::Escape);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_FALSE(holdsStop(events));
}

TEST(StopOnKeySourceTest, EventsFor_IgnoresAReleaseOfTheChosenKey)
{
    const InputEventCodec codec;
    ReplaySource innerSource(
        {TickEvent{
            .tick = 0,
            .event = codec.encode(KeyReleased{.key = Key::Escape})}});
    StopOnKeySource source(innerSource, codec, Key::Escape);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_FALSE(holdsStop(events));
}

TEST(StopOnKeySourceTest, EventsFor_IgnoresEventsThatAreNotInputAtAll)
{
    ReplaySource innerSource(
        {TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}}});
    const InputEventCodec codec;
    StopOnKeySource source(innerSource, codec, Key::Escape);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_FALSE(holdsStop(events));
}

TEST(StopOnKeySourceTest, EventsFor_AppendsOneStopForTwoPressesInATick)
{
    ReplaySource innerSource({keyDown(Key::Escape), keyDown(Key::Escape)});
    const InputEventCodec codec;
    StopOnKeySource source(innerSource, codec, Key::Escape);

    const auto events = source.eventsFor(0);

    std::size_t stops = 0;
    for (const auto &event : events)
    {
        if (event.name == kStop)
        {
            ++stops;
        }
    }

    EXPECT_EQ(stops, 1U);
}

TEST(StopOnKeySourceTest, EventsFor_AddsNothingToAnEmptyTick)
{
    ReplaySource innerSource({});
    const InputEventCodec codec;
    StopOnKeySource source(innerSource, codec, Key::Escape);

    EXPECT_TRUE(source.eventsFor(0).empty());
}
