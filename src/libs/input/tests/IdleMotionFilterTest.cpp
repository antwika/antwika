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

    [[nodiscard]] Event getEncodedMoveEvent(std::int32_t x, std::int32_t y)
    {
        return kCodec.getEncodedEvent(PointerMoved{.position = {.x = x, .y = y}});
    }

    [[nodiscard]] Event getEncodedPressEvent(MouseButton button)
    {
        return kCodec.getEncodedEvent(PointerButtonPressed{.button = button});
    }

    [[nodiscard]] Event getEncodedReleaseEvent(MouseButton button)
    {
        return kCodec.getEncodedEvent(PointerButtonReleased{.button = button});
    }

    [[nodiscard]] TickEvent getEntryAt(Tick tick, Event event)
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
    ReplaySource innerSource({getEntryAt(0, getEncodedMoveEvent(1, 1))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_TRUE(sourceFilter.eventsFor(0).empty());
}

TEST(IdleMotionFilterTest, EventsFor_PassesMovementWhileAButtonIsHeld)
{
    ReplaySource innerSource(
        {getEntryAt(0, getEncodedPressEvent(MouseButton::Middle)),
         getEntryAt(0, getEncodedMoveEvent(1, 1)),
         getEntryAt(0, getEncodedMoveEvent(2, 2))});
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
        {getEntryAt(0, getEncodedMoveEvent(1, 1)),
         getEntryAt(0, getEncodedMoveEvent(2, 2)),
         getEntryAt(0, getEncodedMoveEvent(3, 3)),
         getEntryAt(0, getEncodedPressEvent(MouseButton::Left))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    const auto result = sourceFilter.eventsFor(0);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], getEncodedMoveEvent(3, 3));
    EXPECT_EQ(result[1].name, events::kPointerDown);
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesTheMovementBeforeAScroll)
{
    ReplaySource innerSource(
        {getEntryAt(0, getEncodedMoveEvent(7, 8)),
         getEntryAt(0, kCodec.getEncodedEvent(PointerScrolled{.vertical = 1}))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    const auto result = sourceFilter.eventsFor(0);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], getEncodedMoveEvent(7, 8));
    EXPECT_EQ(result[1].name, events::kPointerScroll);
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesTheMovementBeforeAKey)
{
    ReplaySource innerSource(
        {getEntryAt(0, getEncodedMoveEvent(7, 8)),
         getEntryAt(0, kCodec.getEncodedEvent(KeyPressed{.key = Key::Escape}))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{events::kPointerMove, events::kKeyDown}));
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesTheMovementBeforeAnyOtherEvent)
{
    ReplaySource innerSource(
        {getEntryAt(0, getEncodedMoveEvent(7, 8)), getEntryAt(0, Event{.name = "engine.stop"})});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_EQ(
        namesOf(sourceFilter.eventsFor(0)),
        (std::vector<std::string>{events::kPointerMove, "engine.stop"}));
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesAHeldBackMovementOnALaterTick)
{
    ReplaySource innerSource(
        {getEntryAt(0, getEncodedMoveEvent(1, 1)),
         getEntryAt(1, getEncodedMoveEvent(2, 2)),
         getEntryAt(2, getEncodedPressEvent(MouseButton::Left))});
    IdleMotionFilter sourceFilter(innerSource, kCodec);

    EXPECT_TRUE(sourceFilter.eventsFor(0).empty());
    EXPECT_TRUE(sourceFilter.eventsFor(1).empty());

    const auto result = sourceFilter.eventsFor(2);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], getEncodedMoveEvent(2, 2));
    EXPECT_EQ(result[1].name, events::kPointerDown);
}

TEST(IdleMotionFilterTest, EventsFor_ReleasesAHeldBackMovementOnlyOnce)
{
    ReplaySource innerSource(
        {getEntryAt(0, getEncodedMoveEvent(1, 1)),
         getEntryAt(0, getEncodedPressEvent(MouseButton::Left)),
         getEntryAt(0, getEncodedReleaseEvent(MouseButton::Left))});
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
        {getEntryAt(0, getEncodedPressEvent(MouseButton::Left)),
         getEntryAt(0, getEncodedMoveEvent(1, 1)),
         getEntryAt(0, getEncodedReleaseEvent(MouseButton::Left)),
         getEntryAt(0, getEncodedMoveEvent(2, 2))});
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
        getEntryAt(0, getEncodedMoveEvent(1, 1)),
        getEntryAt(0, getEncodedPressEvent(MouseButton::Middle)),
        getEntryAt(0, getEncodedMoveEvent(2, 2)),
        getEntryAt(1, getEncodedReleaseEvent(MouseButton::Middle)),
        getEntryAt(1, getEncodedMoveEvent(3, 3)),
        getEntryAt(2, kCodec.getEncodedEvent(PointerScrolled{.vertical = -1}))};

    ReplaySource innerSource(streamEvents);
    IdleMotionFilter onceFilter(innerSource, kCodec);

    std::vector<TickEvent> gatedEvents;
    for (Tick tick = 0; tick <= 2; ++tick)
    {
        for (auto &event : onceFilter.eventsFor(tick))
        {
            gatedEvents.push_back(getEntryAt(tick, std::move(event)));
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
            againEvents.push_back(getEntryAt(tick, std::move(event)));
        }
    }

    EXPECT_EQ(againEvents, gatedEvents);
}
