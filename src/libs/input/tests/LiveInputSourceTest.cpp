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
    ReplaySource inner({TickEvent{.tick = 0, .event = kScripted}});
    FakeInputBackend backend;
    const InputEventCodec codec;

    LiveInputSource source(inner, backend, codec);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{kScripted}));
}

TEST(LiveInputSourceTest, EventsFor_ReportsNothingWhenNobodyTouchedAnything)
{
    ReplaySource inner({});
    FakeInputBackend backend;
    const InputEventCodec codec;

    LiveInputSource source(inner, backend, codec);

    EXPECT_TRUE(source.eventsFor(0).empty());
}

TEST(LiveInputSourceTest, EventsFor_AppendsEveryEdgeAfterTheInnerEvents)
{
    ReplaySource inner({TickEvent{.tick = 0, .event = kScripted}});
    FakeInputBackend backend({
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 4, .y = 5}},
        PointerMoved{.position = {.x = 6, .y = 7}},
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 6, .y = 7}},
    });
    const InputEventCodec codec;

    LiveInputSource source(inner, backend, codec);

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
    const Event encoded{.name = "input.made_up"};
    const InputEvent edge = KeyPressed{.key = Key::Space};

    ReplaySource inner({});
    NiceMock<MockInputBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(edge))
        .WillOnce(Return(std::nullopt));

    NiceMock<MockInputEventCodec> codec;
    EXPECT_CALL(codec, encode(edge)).WillOnce(Return(encoded));

    LiveInputSource source(inner, backend, codec);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{encoded}));
}

TEST(LiveInputSourceTest, EventsFor_DrainsTheBackendEveryTick)
{
    ReplaySource inner({});
    NiceMock<MockInputBackend> backend;
    const InputEvent edge = PointerMoved{.position = {.x = 1, .y = 1}};

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(edge))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(edge))
        .WillOnce(Return(std::nullopt));

    const InputEventCodec codec;
    LiveInputSource source(inner, backend, codec);

    EXPECT_EQ(source.eventsFor(0).size(), 1u);
    EXPECT_EQ(source.eventsFor(1).size(), 1u);
}

TEST(LiveInputSourceTest, EventsFor_AsksTheInnerSourceForTheTickItWasGiven)
{
    const Event later{
        .name = "life.toggle_cell", .payload = R"({"x":0,"y":0})"};

    ReplaySource inner({TickEvent{.tick = 3, .event = later}});
    FakeInputBackend backend;
    const InputEventCodec codec;

    LiveInputSource source(inner, backend, codec);

    EXPECT_TRUE(source.eventsFor(0).empty());
    EXPECT_EQ(source.eventsFor(3), (std::vector<Event>{later}));
}
