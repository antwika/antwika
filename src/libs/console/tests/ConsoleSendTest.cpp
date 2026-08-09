#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/console/ConsoleEvents.hpp"
#include "antwika/console/ConsoleGatedSink.hpp"
#include "antwika/console/ConsolePicture.hpp"
#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleSink.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/InputFold.hpp"
#include "antwika/console/fakes/FakeQuietCommands.hpp"
#include "antwika/console/fakes/FakeRefusingSink.hpp"
#include "antwika/console/fakes/FakeRecordingSink.hpp"
#include "antwika/console/testing/ConsoleScript.hpp"

using antwika::console::ConsoleEvents;
using antwika::console::ConsoleGatedSink;
using antwika::console::ConsoleSink;
using antwika::console::ConsoleSinkSetup;
using antwika::console::ConsoleState;
using antwika::console::fakes::FakeQuietCommands;
using antwika::console::fakes::FakeRefusingSink;
using antwika::console::fakes::FakeRecordingSink;
using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::typeText;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::time::Tick;

namespace
{
    constexpr antwika::gfx::Size kCanvas{
        .width = 1024, .height = 640};

    struct Harness final
    {
        explicit Harness(const bool queued = true)
            : sink(ConsoleSinkSetup{
                  .console = console,
                  .input = input,
                  .picture = picture,
                  .scene = scene,
                  .controls = controls,
                  .commands = commands,
                  .events = queued
                      ? std::optional{std::ref(sent)}
                      : std::nullopt})
        {
        }

        InputEventCodec codec;
        antwika::console::InputFold input{codec};
        antwika::console::ConsolePicture picture{kCanvas};
        antwika::console::ConsoleScene scene;
        ConsoleState console;
        antwika::console::FixedConsoleControls controls;
        FakeQuietCommands commands;
        ConsoleEvents sent;
        std::size_t reader{sent.open()};
        ConsoleSink sink;

        void feed(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void type(const std::string_view line)
        {
            feed(keyAt(codec, 1, Key::Grave));

            for (Tick tick = 1; tick <= kConsoleAnimTicks; ++tick)
            {
                feed(TickEvent{
                    .tick = tick,
                    .event = Event{
                        .name = antwika::engine::events::kTick}});
            }

            std::vector<TickEvent> typed;
            typeText(typed, codec, kOpenTick, line);
            typed.push_back(keyAt(codec, kOpenTick, Key::Enter));

            for (const auto &event : typed)
            {
                feed(event);
            }
        }

        [[nodiscard]] std::string lastSaid() const
        {
            return console.history().back();
        }
    };
}

TEST(ConsoleSendTest, Send_QueuesTheEventTheLineNames)
{
    Harness harness;

    harness.type("send spawn");

    ASSERT_EQ(harness.sent.pending(), 1U);
    EXPECT_EQ(harness.lastSaid(), "sent spawn");

    const auto queued = harness.sent.take(harness.reader);

    ASSERT_EQ(queued.size(), 1U);
    EXPECT_EQ(queued.front().name, "spawn");
    EXPECT_TRUE(queued.front().payload.empty());
}

TEST(ConsoleSendTest, Send_CarriesTheJsonPayloadTheLineGaveIt)
{
    Harness harness;

    harness.type("send spawn {\"at\": 3}");

    const auto queued = harness.sent.take(harness.reader);

    ASSERT_EQ(queued.size(), 1U);
    EXPECT_EQ(queued.front().name, "spawn");
    EXPECT_EQ(queued.front().payload, "{\"at\": 3}");
}

TEST(ConsoleSendTest, Send_RefusesAPayloadThatIsNotJson)
{
    Harness harness;

    harness.type("send spawn nonsense");

    EXPECT_EQ(harness.sent.pending(), 0U);
    EXPECT_EQ(harness.lastSaid(), "send: the payload is not json");
}

TEST(ConsoleSendTest, Send_RefusesALineThatNamesNoEvent)
{
    Harness harness;

    harness.type("send");

    EXPECT_EQ(harness.sent.pending(), 0U);
    EXPECT_EQ(harness.lastSaid(), "send: name the event to send");
}

TEST(ConsoleSendTest, Send_SaysSoWithNoQueueToSendThrough)
{
    Harness harness(false);

    harness.type("send spawn");

    EXPECT_EQ(harness.sent.pending(), 0U);
    EXPECT_EQ(
        harness.lastSaid(), "send: this run carries no event queue");
}

TEST(ConsoleSendTest, Handle_DeliversASentEventToTheSinkItGates)
{
    Harness harness;
    FakeRecordingSink inner;
    ConsoleGatedSink gate{
        inner, harness.console, harness.input, harness.sent};

    harness.type("send spawn");

    gate.handle(TickEvent{
        .tick = kOpenTick + 1,
        .event = Event{.name = antwika::engine::events::kTick}});

    ASSERT_EQ(inner.seen.size(), 1U);
    EXPECT_EQ(inner.seen.front().name, "spawn");

    gate.handle(TickEvent{
        .tick = kOpenTick + 2,
        .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(inner.seen.size(), 1U);
}

TEST(ConsoleEventsTest, Take_LeavesTheQueueEmptyBehindOneReader)
{
    ConsoleEvents events;
    const auto reader = events.open();

    EXPECT_EQ(events.pending(), 0U);
    EXPECT_TRUE(events.take(reader).empty());

    events.send(Event{.name = "one"});
    events.send(Event{.name = "two"});

    EXPECT_EQ(events.pending(), 2U);

    const auto taken = events.take(reader);

    ASSERT_EQ(taken.size(), 2U);
    EXPECT_EQ(taken[0].name, "one");
    EXPECT_EQ(taken[1].name, "two");
    EXPECT_EQ(events.pending(), 0U);
}

TEST(ConsoleEventsTest, Take_HandsTheSameEventToEveryReaderThatOpened)
{
    ConsoleEvents events;
    const auto first = events.open();
    const auto second = events.open();

    events.send(Event{.name = "one"});

    const auto toFirst = events.take(first);

    ASSERT_EQ(toFirst.size(), 1U);
    EXPECT_EQ(events.pending(), 1U);

    const auto toSecond = events.take(second);

    ASSERT_EQ(toSecond.size(), 1U);
    EXPECT_EQ(toSecond.front().name, "one");
    EXPECT_EQ(events.pending(), 0U);
}

TEST(ConsoleEventsTest, Open_StartsAReaderPastWhatWasAlreadySent)
{
    ConsoleEvents events;
    const auto first = events.open();

    events.send(Event{.name = "one"});

    const auto late = events.open();

    EXPECT_TRUE(events.take(late).empty());
    EXPECT_EQ(events.take(first).size(), 1U);
    EXPECT_EQ(events.pending(), 0U);
}

TEST(ConsoleSendTest, Handle_SaysWhyASinkRefusedTheEventItWasSent)
{
    Harness harness;
    FakeRefusingSink inner{"no schema fits that"};
    ConsoleGatedSink gate{
        inner, harness.console, harness.input, harness.sent};

    harness.type("send bogus");

    gate.handle(TickEvent{
        .tick = kOpenTick + 1,
        .event = Event{.name = antwika::engine::events::kTick}});

    harness.feed(TickEvent{
        .tick = kOpenTick + 2,
        .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(
        harness.lastSaid(), "bogus refused: no schema fits that");
}

TEST(ConsoleEventsTest, TakeRefusals_LeavesTheRefusalsBehindIt)
{
    ConsoleEvents events;

    EXPECT_TRUE(events.takeRefusals().empty());

    events.refuse("bogus", "no schema fits that");

    const auto taken = events.takeRefusals();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_EQ(taken.front(), "bogus refused: no schema fits that");
    EXPECT_TRUE(events.takeRefusals().empty());
}

TEST(ConsoleSendTest, Handle_PutsAPastCommandBackInTheFieldOnArrowUp)
{
    Harness harness;

    harness.type("send one");

    harness.feed(keyAt(harness.codec, kOpenTick, Key::ArrowUp));

    EXPECT_EQ(harness.console.line(), "send one");

    harness.feed(keyAt(harness.codec, kOpenTick, Key::ArrowDown));

    EXPECT_TRUE(harness.console.line().empty());
}
