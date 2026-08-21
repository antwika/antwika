#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/PointerHintSource.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/PointerHint.hpp"
#include "antwika/input/PointerHintChannel.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerHint;
using antwika::input::PointerHintChannel;
using antwika::input::PointerHintSource;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::replay::ReplaySource;

namespace
{
    const InputEventCodec kCodec;

    [[nodiscard]] TickEvent at(antwika::time::Tick tick, InputEvent edgeEvent)
    {
        return TickEvent{.tick = tick, .event = kCodec.encode(edgeEvent)};
    }

    [[nodiscard]] InputEvent moved(std::int32_t x, std::int32_t y)
    {
        return PointerMoved{.position = {.x = x, .y = y}};
    }

    [[nodiscard]] PointerHint hint(std::int32_t x, std::int32_t y)
    {
        return PointerHint{.position = {.x = x, .y = y}};
    }

    void run(PointerHintSource &source, antwika::time::Tick tick)
    {
        [[maybe_unused]] const auto events = source.eventsFor(tick);
    }
}

TEST(PointerHintSourceTest, EventsFor_ReturnsTheInnerEventsUntouched)
{
    ReplaySource innerSource(
        {at(0, moved(1, 1)),
         TickEvent{.tick = 0, .event = Event{.name = "game.score_increment"}},
         at(0, moved(2, 2))});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    EXPECT_EQ(
        source.eventsFor(0),
        (std::vector<Event>{
            kCodec.encode(moved(1, 1)),
            Event{.name = "game.score_increment"},
            kCodec.encode(moved(2, 2))}));
}

TEST(PointerHintSourceTest, EventsFor_PublishesThePositionAMovementCarried)
{
    ReplaySource innerSource({at(0, moved(4, 5))});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.latest(), hint(4, 5));
}

TEST(PointerHintSourceTest, EventsFor_PublishesThePositionAPressCarried)
{
    ReplaySource innerSource(
        {at(0,
            PointerButtonPressed{
                .button = MouseButton::Left, .position = {.x = 6, .y = 7}})});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.latest(), hint(6, 7));
}

TEST(PointerHintSourceTest, EventsFor_PublishesThePositionAReleaseCarried)
{
    ReplaySource innerSource(
        {at(0,
            PointerButtonReleased{
                .button = MouseButton::Left, .position = {.x = 8, .y = 9}})});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.latest(), hint(8, 9));
}

TEST(PointerHintSourceTest, EventsFor_PublishesTheLastPositionOfTheTick)
{
    ReplaySource innerSource(
        {at(0, moved(1, 1)), at(0, moved(2, 2)), at(0, moved(3, 3))});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.latest(), hint(3, 3));
}

TEST(PointerHintSourceTest, EventsFor_PublishesNothingBeforeAPositionArrives)
{
    ReplaySource innerSource(
        {at(0, PointerScrolled{.vertical = 1}),
         at(0, KeyPressed{.key = Key::A})});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.latest(), std::nullopt);
}

TEST(PointerHintSourceTest, EventsFor_LeavesTheHintAloneOnAPositionlessTick)
{
    ReplaySource innerSource(
        {at(0, moved(4, 5)), at(1, PointerScrolled{.vertical = 1})});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);
    run(source, 1);

    EXPECT_EQ(channel.latest(), hint(4, 5));
}

TEST(PointerHintSourceTest, EventsFor_IgnoresAnEventThatIsNotInput)
{
    ReplaySource innerSource(
        {TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}}});
    PointerHintChannel channel;
    PointerHintSource source(innerSource, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.latest(), std::nullopt);
}
