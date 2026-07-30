#include <gtest/gtest.h>

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/input/CoalescingPointerSource.hpp"
#include "antwika/input/Events.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/MouseButton.hpp"

using antwika::event::Event;
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

    [[nodiscard]] TickEvent moveTo(std::int32_t x, std::int32_t y)
    {
        return TickEvent{
            .tick = 0,
            .event = kCodec.encode(
                PointerMoved{.position = {.x = x, .y = y}})};
    }

    [[nodiscard]] TickEvent click()
    {
        return TickEvent{
            .tick = 0,
            .event =
                kCodec.encode(
                    PointerButtonPressed{.button = MouseButton::Left})};
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
} // namespace

TEST(CoalescingPointerSourceTest, EventsFor_KeepsOnlyTheLastOfARun)
{
    ReplaySource inner({moveTo(1, 1), moveTo(2, 2), moveTo(3, 3)});
    CoalescingPointerSource source(inner);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(
        events[0],
        kCodec.encode(PointerMoved{.position = {.x = 3, .y = 3}}));
}

TEST(CoalescingPointerSourceTest, EventsFor_KeepsASingleMovement)
{
    ReplaySource inner({moveTo(4, 5)});
    CoalescingPointerSource source(inner);

    EXPECT_EQ(source.eventsFor(0).size(), 1U);
}

TEST(CoalescingPointerSourceTest, EventsFor_KeepsTheMovementBeforeAClick)
{
    // Dropping this one would move the click.
    ReplaySource inner({moveTo(1, 1), click(), moveTo(9, 9)});
    CoalescingPointerSource source(inner);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerMove,
            events::kPointerDown,
            events::kPointerMove}));
}

TEST(CoalescingPointerSourceTest, EventsFor_ThinsEachRunSeparately)
{
    ReplaySource inner(
        {moveTo(1, 1),
         moveTo(2, 2),
         click(),
         moveTo(3, 3),
         moveTo(4, 4)});
    CoalescingPointerSource source(inner);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(
        events[0],
        kCodec.encode(PointerMoved{.position = {.x = 2, .y = 2}}));
    EXPECT_EQ(events[1].name, events::kPointerDown);
    EXPECT_EQ(
        events[2],
        kCodec.encode(PointerMoved{.position = {.x = 4, .y = 4}}));
}

TEST(CoalescingPointerSourceTest, EventsFor_LeavesEverythingElseAlone)
{
    ReplaySource inner(
        {TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
         click()});
    CoalescingPointerSource source(inner);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{"engine.tick", events::kPointerDown}));
}

TEST(CoalescingPointerSourceTest, EventsFor_PassesAnEmptyTickThrough)
{
    ReplaySource inner({});
    CoalescingPointerSource source(inner);

    EXPECT_TRUE(source.eventsFor(0).empty());
}
