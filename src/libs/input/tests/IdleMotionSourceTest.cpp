#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/Events.hpp"
#include "antwika/input/IdleMotionSource.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::IdleMotionSource;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
namespace events = antwika::input::events;

namespace
{
    const InputEventCodec kCodec;

    [[nodiscard]] Event move(std::int32_t x, std::int32_t y)
    {
        return kCodec.encode(PointerMoved{.position = {.x = x, .y = y}});
    }

    [[nodiscard]] Event press(MouseButton button)
    {
        return kCodec.encode(PointerButtonPressed{.button = button});
    }

    [[nodiscard]] Event release(MouseButton button)
    {
        return kCodec.encode(PointerButtonReleased{.button = button});
    }

    [[nodiscard]] TickEvent at(Tick tick, Event event)
    {
        return TickEvent{.tick = tick, .event = std::move(event)};
    }

    [[nodiscard]] std::vector<std::string> namesOf(
        const std::vector<Event> &events)
    {
        std::vector<std::string> names;
        for (const auto &event : events)
        {
            names.push_back(event.name);
        }
        return names;
    }
}

TEST(IdleMotionSourceTest, EventsFor_HoldsBackMovementWithNoButtonHeld)
{
    ReplaySource inner({at(0, move(1, 1))});
    IdleMotionSource source(inner, kCodec);

    EXPECT_TRUE(source.eventsFor(0).empty());
}

TEST(IdleMotionSourceTest, EventsFor_PassesMovementWhileAButtonIsHeld)
{
    ReplaySource inner(
        {at(0, press(MouseButton::Middle)),
         at(0, move(1, 1)),
         at(0, move(2, 2))});
    IdleMotionSource source(inner, kCodec);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerDown,
            events::kPointerMove,
            events::kPointerMove}));
}

TEST(IdleMotionSourceTest, EventsFor_ReleasesOnlyTheLatestHeldBackMovement)
{
    ReplaySource inner(
        {at(0, move(1, 1)),
         at(0, move(2, 2)),
         at(0, move(3, 3)),
         at(0, press(MouseButton::Left))});
    IdleMotionSource source(inner, kCodec);

    const auto result = source.eventsFor(0);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], move(3, 3));
    EXPECT_EQ(result[1].name, events::kPointerDown);
}

TEST(IdleMotionSourceTest, EventsFor_ReleasesTheMovementBeforeAScroll)
{
    ReplaySource inner(
        {at(0, move(7, 8)),
         at(0, kCodec.encode(PointerScrolled{.vertical = 1}))});
    IdleMotionSource source(inner, kCodec);

    const auto result = source.eventsFor(0);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], move(7, 8));
    EXPECT_EQ(result[1].name, events::kPointerScroll);
}

TEST(IdleMotionSourceTest, EventsFor_ReleasesTheMovementBeforeAKey)
{
    ReplaySource inner(
        {at(0, move(7, 8)),
         at(0, kCodec.encode(KeyPressed{.key = Key::Escape}))});
    IdleMotionSource source(inner, kCodec);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{events::kPointerMove, events::kKeyDown}));
}

TEST(IdleMotionSourceTest, EventsFor_ReleasesTheMovementBeforeAnyOtherEvent)
{
    ReplaySource inner(
        {at(0, move(7, 8)), at(0, Event{.name = "engine.stop"})});
    IdleMotionSource source(inner, kCodec);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{events::kPointerMove, "engine.stop"}));
}

TEST(IdleMotionSourceTest, EventsFor_ReleasesAHeldBackMovementOnALaterTick)
{
    ReplaySource inner(
        {at(0, move(1, 1)),
         at(1, move(2, 2)),
         at(2, press(MouseButton::Left))});
    IdleMotionSource source(inner, kCodec);

    EXPECT_TRUE(source.eventsFor(0).empty());
    EXPECT_TRUE(source.eventsFor(1).empty());

    const auto result = source.eventsFor(2);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], move(2, 2));
    EXPECT_EQ(result[1].name, events::kPointerDown);
}

TEST(IdleMotionSourceTest, EventsFor_ReleasesAHeldBackMovementOnlyOnce)
{
    ReplaySource inner(
        {at(0, move(1, 1)),
         at(0, press(MouseButton::Left)),
         at(0, release(MouseButton::Left))});
    IdleMotionSource source(inner, kCodec);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerMove,
            events::kPointerDown,
            events::kPointerUp}));
}

TEST(IdleMotionSourceTest, EventsFor_HoldsBackMovementAgainAfterARelease)
{
    ReplaySource inner(
        {at(0, press(MouseButton::Left)),
         at(0, move(1, 1)),
         at(0, release(MouseButton::Left)),
         at(0, move(2, 2))});
    IdleMotionSource source(inner, kCodec);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerDown,
            events::kPointerMove,
            events::kPointerUp}));
}

TEST(IdleMotionSourceTest, EventsFor_PassesAnEmptyTickThrough)
{
    ReplaySource inner({});
    IdleMotionSource source(inner, kCodec);

    EXPECT_TRUE(source.eventsFor(0).empty());
}

TEST(IdleMotionSourceTest, EventsFor_LeavesAnAlreadyGatedStreamAlone)
{
    const std::vector<TickEvent> stream{
        at(0, move(1, 1)),
        at(0, press(MouseButton::Middle)),
        at(0, move(2, 2)),
        at(1, release(MouseButton::Middle)),
        at(1, move(3, 3)),
        at(2, kCodec.encode(PointerScrolled{.vertical = -1}))};

    ReplaySource inner(stream);
    IdleMotionSource once(inner, kCodec);

    std::vector<TickEvent> gated;
    for (Tick tick = 0; tick <= 2; ++tick)
    {
        for (auto &event : once.eventsFor(tick))
        {
            gated.push_back(at(tick, std::move(event)));
        }
    }

    ASSERT_EQ(gated.size(), stream.size());

    ReplaySource gatedInner(gated);
    IdleMotionSource twice(gatedInner, kCodec);

    std::vector<TickEvent> again;
    for (Tick tick = 0; tick <= 2; ++tick)
    {
        for (auto &event : twice.eventsFor(tick))
        {
            again.push_back(at(tick, std::move(event)));
        }
    }

    EXPECT_EQ(again, gated);
}
