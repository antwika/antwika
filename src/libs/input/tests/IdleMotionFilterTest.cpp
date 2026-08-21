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
#include "antwika/input/IdleMotionFilter.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::IdleMotionFilter;
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

TEST(IdleMotionFilterTest, EventsFor_HoldsBackMovementWithNoButtonHeld)
{
    ReplaySource innerSource({at(0, move(1, 1))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_TRUE(sourceFilter.eventsFor(0).empty());
}

TEST(IdleMotionFilterTest, EventsFor_PassesMovementWhileAButtonIsHeld)
{
    ReplaySource innerSource(
        {at(0, press(MouseButton::Middle)),
         at(0, move(1, 1)),
         at(0, move(2, 2))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerDown,
            events::kPointerMove,
            events::kPointerMove}));
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesOnlyTheLatestHeldBackMovement)
{
    ReplaySource innerSource(
        {at(0, move(1, 1)),
         at(0, move(2, 2)),
         at(0, move(3, 3)),
         at(0, press(MouseButton::Left))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    const auto result = sourceFilter.eventsFor(0);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], move(3, 3));
    EXPECT_EQ(result[1].name, events::kPointerDown);
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesTheMovementBeforeAScroll)
{
    ReplaySource innerSource(
        {at(0, move(7, 8)),
         at(0, kCodec.encode(PointerScrolled{.vertical = 1}))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    const auto result = sourceFilter.eventsFor(0);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], move(7, 8));
    EXPECT_EQ(result[1].name, events::kPointerScroll);
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesTheMovementBeforeAKey)
{
    ReplaySource innerSource(
        {at(0, move(7, 8)),
         at(0, kCodec.encode(KeyPressed{.key = Key::Escape}))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{events::kPointerMove, events::kKeyDown}));
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesTheMovementBeforeAnyOtherEvent)
{
    ReplaySource innerSource(
        {at(0, move(7, 8)), at(0, Event{.name = "engine.stop"})});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{events::kPointerMove, "engine.stop"}));
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesAHeldBackMovementOnALaterTick)
{
    ReplaySource innerSource(
        {at(0, move(1, 1)),
         at(1, move(2, 2)),
         at(2, press(MouseButton::Left))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_TRUE(sourceFilter.eventsFor(0).empty());
    EXPECT_TRUE(sourceFilter.eventsFor(1).empty());

    const auto result = sourceFilter.eventsFor(2);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], move(2, 2));
    EXPECT_EQ(result[1].name, events::kPointerDown);
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesAHeldBackMovementOnlyOnce)
{
    ReplaySource innerSource(
        {at(0, move(1, 1)),
         at(0, press(MouseButton::Left)),
         at(0, release(MouseButton::Left))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerMove,
            events::kPointerDown,
            events::kPointerUp}));
}

TEST(IdleMotionFilterTest, EventsFor_HoldsBackMovementAgainAfterARelease)
{
    ReplaySource innerSource(
        {at(0, press(MouseButton::Left)),
         at(0, move(1, 1)),
         at(0, release(MouseButton::Left)),
         at(0, move(2, 2))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerDown,
            events::kPointerMove,
            events::kPointerUp}));
}

TEST(IdleMotionFilterTest, EventsFor_PassesAnEmptyTickThrough)
{
    ReplaySource innerSource({});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_TRUE(sourceFilter.eventsFor(0).empty());
}

TEST(IdleMotionFilterTest, EventsFor_LeavesAnAlreadyGatedStreamAlone)
{
    const std::vector<TickEvent> streamEvents{
        at(0, move(1, 1)),
        at(0, press(MouseButton::Middle)),
        at(0, move(2, 2)),
        at(1, release(MouseButton::Middle)),
        at(1, move(3, 3)),
        at(2, kCodec.encode(PointerScrolled{.vertical = -1}))};

    ReplaySource innerSource(streamEvents);
    IdleMotionFilter onceFilter(innerSource, kCodec);

    std::vector<TickEvent> gatedEvents;
    for (Tick tick = 0; tick <= 2; ++tick)
    {
        for (auto &event : onceFilter.eventsFor(tick))
        {
            gatedEvents.push_back(at(tick, std::move(event)));
        }
    }

    ASSERT_EQ(gatedEvents.size(), streamEvents.size());

    ReplaySource gatedInnerSource(gatedEvents);
    IdleMotionFilter twiceFilter(gatedInnerSource, kCodec);

    std::vector<TickEvent> againEvents;
    for (Tick tick = 0; tick <= 2; ++tick)
    {
        for (auto &event : twiceFilter.eventsFor(tick))
        {
            againEvents.push_back(at(tick, std::move(event)));
        }
    }

    EXPECT_EQ(againEvents, gatedEvents);
}
