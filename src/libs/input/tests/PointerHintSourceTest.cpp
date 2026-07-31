#include "antwika/input/PointerHintSource.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

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

    [[nodiscard]] TickEvent at(antwika::time::Tick tick, InputEvent edge)
    {
        return TickEvent{.tick = tick, .event = kCodec.encode(edge)};
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
} // namespace

TEST(PointerHintSourceTest, EventsFor_ReturnsTheInnerEventsUntouched)
{
    // The whole guarantee of this class, and what makes it free to add.
    // A recording is a function of the stream this cannot alter.
    ReplaySource inner(
        {at(0, moved(1, 1)),
         TickEvent{.tick = 0, .event = Event{.name = "game.score_increment"}},
         at(0, moved(2, 2))});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    EXPECT_EQ(
        source.eventsFor(0),
        (std::vector<Event>{
            kCodec.encode(moved(1, 1)),
            Event{.name = "game.score_increment"},
            kCodec.encode(moved(2, 2))}));
}

TEST(PointerHintSourceTest, EventsFor_PublishesThePositionAMovementCarried)
{
    ReplaySource inner({at(0, moved(4, 5))});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.forRenderingOnly(), hint(4, 5));
}

TEST(PointerHintSourceTest, EventsFor_PublishesThePositionAPressCarried)
{
    ReplaySource inner(
        {at(0,
            PointerButtonPressed{
                .button = MouseButton::Left, .position = {.x = 6, .y = 7}})});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.forRenderingOnly(), hint(6, 7));
}

TEST(PointerHintSourceTest, EventsFor_PublishesThePositionAReleaseCarried)
{
    ReplaySource inner(
        {at(0,
            PointerButtonReleased{
                .button = MouseButton::Left, .position = {.x = 8, .y = 9}})});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.forRenderingOnly(), hint(8, 9));
}

TEST(PointerHintSourceTest, EventsFor_PublishesTheLastPositionOfTheTick)
{
    // Where the pointer is, not a sum of where it went.
    ReplaySource inner(
        {at(0, moved(1, 1)), at(0, moved(2, 2)), at(0, moved(3, 3))});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.forRenderingOnly(), hint(3, 3));
}

TEST(PointerHintSourceTest, EventsFor_PublishesNothingBeforeAPositionArrives)
{
    // A wheel notch and a key both happen wherever the pointer was.
    // Neither says where that is, so neither may invent the origin.
    ReplaySource inner(
        {at(0, PointerScrolled{.vertical = 1}),
         at(0, KeyPressed{.key = Key::A})});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.forRenderingOnly(), std::nullopt);
}

TEST(PointerHintSourceTest, EventsFor_LeavesTheHintAloneOnAPositionlessTick)
{
    ReplaySource inner(
        {at(0, moved(4, 5)), at(1, PointerScrolled{.vertical = 1})});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);
    run(source, 1);

    // Still where the pointer is, since a scroll did not move it.
    EXPECT_EQ(channel.forRenderingOnly(), hint(4, 5));
}

TEST(PointerHintSourceTest, EventsFor_IgnoresAnEventThatIsNotInput)
{
    ReplaySource inner(
        {TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}}});
    PointerHintChannel channel;
    PointerHintSource source(inner, kCodec, channel);

    run(source, 0);

    EXPECT_EQ(channel.forRenderingOnly(), std::nullopt);
}
