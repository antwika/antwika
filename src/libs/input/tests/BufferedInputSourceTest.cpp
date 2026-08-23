#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/input/BufferedInputSource.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/LiveInputSource.hpp"

using antwika::event::Event;
using antwika::input::BufferedInputSource;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::LiveInputSource;
using antwika::input::PointerMoved;
using antwika::input::fakes::FakeInputBackend;
using antwika::replay::ReplaySource;

namespace
{
    const InputEventCodec kCodec;

    [[nodiscard]] InputEvent getPointerMoveEvent(std::int32_t x, std::int32_t y)
    {
        return PointerMoved{.position = {.x = x, .y = y}};
    }

    [[nodiscard]] Event getEncodedMoveEvent(std::int32_t x, std::int32_t y)
    {
        return kCodec.getEncodedEvent(getPointerMoveEvent(x, y));
    }
}

TEST(BufferedInputSourceTest, EventsFor_HandsBackWhatEveryPumpRead)
{
    const std::vector<std::vector<InputEvent>> roundEvents{
        {getPointerMoveEvent(1, 1)}, {getPointerMoveEvent(2, 2)}, {getPointerMoveEvent(3, 3)}};

    ReplaySource nothingScriptedSource({});
    FakeInputBackend backend(roundEvents);
    LiveInputSource liveSource(nothingScriptedSource, backend, kCodec);
    BufferedInputSource bufferedSource(liveSource);

    bufferedSource.pollFrame(0);
    bufferedSource.pollFrame(0);

    EXPECT_EQ(
        bufferedSource.eventsFor(0),
        (std::vector<Event>{
            getEncodedMoveEvent(1, 1),
            getEncodedMoveEvent(2, 2),
            getEncodedMoveEvent(3, 3)}));
}

TEST(BufferedInputSourceTest, EventsFor_ReadsTheSourceItselfWhenNothingPumped)
{
    const std::vector<std::vector<InputEvent>> roundEvents{{getPointerMoveEvent(4, 5)}};

    ReplaySource nothingScriptedSource({});
    FakeInputBackend backend(roundEvents);
    LiveInputSource liveSource(nothingScriptedSource, backend, kCodec);
    BufferedInputSource bufferedSource(liveSource);

    EXPECT_EQ(bufferedSource.eventsFor(0), (std::vector<Event>{getEncodedMoveEvent(4, 5)}));
}

TEST(BufferedInputSourceTest, EventsFor_KeepsNothingBackForTheNextTick)
{
    const std::vector<std::vector<InputEvent>> roundEvents{{getPointerMoveEvent(1, 1)}, {}};

    ReplaySource nothingScriptedSource({});
    FakeInputBackend backend(roundEvents);
    LiveInputSource liveSource(nothingScriptedSource, backend, kCodec);
    BufferedInputSource bufferedSource(liveSource);

    bufferedSource.pollFrame(0);

    ASSERT_FALSE(bufferedSource.eventsFor(0).empty());
    EXPECT_TRUE(bufferedSource.eventsFor(1).empty());
}
