#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/Events.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/fakes/FakeInputBackend.hpp"
#include "antwika/input/mocks/MockInputBackend.hpp"
#include "antwika/input/mocks/MockInputEventCodec.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::LiveInputSource;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::fakes::FakeInputBackend;
using antwika::input::mocks::MockInputBackend;
using antwika::input::mocks::MockInputEventCodec;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;
using ::testing::Return;

namespace events = antwika::input::events;

namespace
{
    const Event kScripted{
        .name = "life.toggle_cell", .payload = R"({"x":1,"y":2})"};

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

TEST(LiveInputSourceTest, EventsFor_ForwardsTheInnerSourcesEventsUnchanged)
{
    ReplaySource innerSource({TickEvent{.tick = 0, .event = kScripted}});
    FakeInputBackend backend;
    const InputEventCodec codec;

    LiveInputSource source(innerSource, backend, codec);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{kScripted}));
}

TEST(LiveInputSourceTest, EventsFor_ReportsNothingWhenNobodyTouchedAnything)
{
    ReplaySource innerSource({});
    FakeInputBackend backend;
    const InputEventCodec codec;

    LiveInputSource source(innerSource, backend, codec);

    EXPECT_TRUE(source.eventsFor(0).empty());
}

TEST(LiveInputSourceTest, EventsFor_AppendsEveryEdgeAfterTheInnerEvents)
{
    ReplaySource innerSource({TickEvent{.tick = 0, .event = kScripted}});
    FakeInputBackend backend({
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 4, .y = 5}},
        PointerMoved{.position = {.x = 6, .y = 7}},
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 6, .y = 7}},
    });
    const InputEventCodec codec;

    LiveInputSource source(innerSource, backend, codec);

    EXPECT_EQ(
        namesOf(source.eventsFor(0)),
        (std::vector<std::string>{
            kScripted.name,
            events::kPointerDown,
            events::kPointerMove,
            events::kPointerUp}));
}

TEST(LiveInputSourceTest, EventsFor_EncodesThroughTheCodecItWasGiven)
{
    const Event encodedEvent{.name = "input.made_up"};
    const InputEvent edgeEvent = KeyPressed{.key = Key::Space};

    ReplaySource innerSource({});
    NiceMock<MockInputBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(edgeEvent))
        .WillOnce(Return(std::nullopt));

    NiceMock<MockInputEventCodec> codec;
    EXPECT_CALL(codec, getEncodedEvent(edgeEvent)).WillOnce(Return(encodedEvent));

    LiveInputSource source(innerSource, backend, codec);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{encodedEvent}));
}

TEST(LiveInputSourceTest, EventsFor_DrainsTheBackendEveryTick)
{
    ReplaySource innerSource({});
    NiceMock<MockInputBackend> backend;
    const InputEvent edgeEvent = PointerMoved{.position = {.x = 1, .y = 1}};

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(edgeEvent))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(edgeEvent))
        .WillOnce(Return(std::nullopt));

    const InputEventCodec codec;
    LiveInputSource source(innerSource, backend, codec);

    EXPECT_EQ(source.eventsFor(0).size(), 1u);
    EXPECT_EQ(source.eventsFor(1).size(), 1u);
}

TEST(LiveInputSourceTest, EventsFor_AsksTheInnerSourceForTheTickItWasGiven)
{
    const Event laterEvent{
        .name = "life.toggle_cell", .payload = R"({"x":0,"y":0})"};

    ReplaySource innerSource({TickEvent{.tick = 3, .event = laterEvent}});
    FakeInputBackend backend;
    const InputEventCodec codec;

    LiveInputSource source(innerSource, backend, codec);

    EXPECT_TRUE(source.eventsFor(0).empty());
    EXPECT_EQ(source.eventsFor(3), (std::vector<Event>{laterEvent}));
}
