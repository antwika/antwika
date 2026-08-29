#include <gtest/gtest.h>

#include <vector>

#include <antwika/event/EngineEvents.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/input/CoalescingPointerSource.hpp"
#include "antwika/input/Events.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/MouseButton.hpp"

using antwika::event::Event;
using antwika::event::kTick;
using antwika::event::EventName;
using antwika::event::TickEvent;
using antwika::input::CoalescingPointerSource;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerMoved;
using antwika::replay::ReplaySource;
namespace events = antwika::input::events;

namespace
{
    const InputEventCodec kCodec;

    [[nodiscard]] TickEvent getMoveTo(std::int32_t x, std::int32_t y)
    {
        return TickEvent{
            .tick = 0,
            .event = kCodec.getEncodedEvent(
                PointerMoved{.position = {.x = x, .y = y}})};
    }

    [[nodiscard]] TickEvent getClickEvent()
    {
        return TickEvent{
            .tick = 0,
            .event =
                kCodec.getEncodedEvent(
                    PointerButtonPressed{.button = MouseButton::Left})};
    }

    [[nodiscard]] std::vector<EventName> namesOf(
        const std::vector<Event> &events)
    {
        std::vector<EventName> names;
        for (const auto &event : events)
        {
            names.push_back(event.name);
        }
        return names;
    }
}

TEST(CoalescingPointerSourceTest, EventsFor_KeepsOnlyTheLastOfARun)
{
    ReplaySource innerSource({getMoveTo(1, 1), getMoveTo(2, 2), getMoveTo(3, 3)});
    CoalescingPointerSource source(innerSource);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(
        events[0],
        kCodec.getEncodedEvent(PointerMoved{.position = {.x = 3, .y = 3}}));
}

TEST(CoalescingPointerSourceTest, EventsFor_KeepsASingleMovement)
{
    ReplaySource innerSource({getMoveTo(4, 5)});
    CoalescingPointerSource source(innerSource);

    EXPECT_EQ(source.eventsFor(0).size(), 1U);
}

TEST(CoalescingPointerSourceTest, EventsFor_KeepsTheMovementBeforeAClick)
{
    ReplaySource innerSource({getMoveTo(1, 1), getClickEvent(), getMoveTo(9, 9)});
    CoalescingPointerSource source(innerSource);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<EventName>{
            events::kPointerMove,
            events::kPointerDown,
            events::kPointerMove}));
}

TEST(CoalescingPointerSourceTest, EventsFor_ThinsEachRunSeparately)
{
    ReplaySource innerSource(
        {getMoveTo(1, 1),
         getMoveTo(2, 2),
         getClickEvent(),
         getMoveTo(3, 3),
         getMoveTo(4, 4)});
    CoalescingPointerSource source(innerSource);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(
        events[0],
        kCodec.getEncodedEvent(PointerMoved{.position = {.x = 2, .y = 2}}));
    EXPECT_EQ(events[1].name, events::kPointerDown);
    EXPECT_EQ(
        events[2],
        kCodec.getEncodedEvent(PointerMoved{.position = {.x = 4, .y = 4}}));
}

TEST(CoalescingPointerSourceTest, EventsFor_LeavesEverythingElseAlone)
{
    ReplaySource innerSource(
        {TickEvent{.tick = 0, .event = Event{.name = kTick}},
         getClickEvent()});
    CoalescingPointerSource source(innerSource);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<EventName>{kTick, events::kPointerDown}));
}

TEST(CoalescingPointerSourceTest, EventsFor_PassesAnEmptyTickThrough)
{
    ReplaySource innerSource({});
    CoalescingPointerSource source(innerSource);

    EXPECT_TRUE(source.eventsFor(0).empty());
}
