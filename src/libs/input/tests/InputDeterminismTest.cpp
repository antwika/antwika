#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <unistd.h>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/EngineLoop.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/input/fakes/FakeSummarySink.hpp>
#include <antwika/input/fakes/SessionSummary.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/testing/ScratchFile.hpp>

#include "antwika/input/IdleMotionFilter.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"
#include "antwika/input/fakes/FakeInputBackend.hpp"

using antwika::engine::Engine;
using antwika::engine::EngineLoop;
using antwika::engine::StopSignal;
using antwika::event::EventDispatcher;
using antwika::event::ITickEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::event::TickedEventDispatcher;
using antwika::input::IdleMotionFilter;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::InputState;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::LiveInputSource;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::input::fakes::FakeSummarySink;
using antwika::input::fakes::SessionSummary;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;

namespace
{
    [[nodiscard]] std::vector<InputEvent> getScriptedSession()
    {
        return {
            PointerMoved{.position = {.x = 100, .y = 100}},
            PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = 100, .y = 100}},
            PointerMoved{.position = {.x = 140, .y = 90}},
            PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = 140, .y = 90}},
            PointerScrolled{.vertical = 3},
            KeyPressed{.key = Key::W, .modifiers = {.shift = true}},
            KeyReleased{.key = Key::W},
            PointerButtonPressed{
                .button = MouseButton::Right,
                .position = {.x = 200, .y = 10}},
        };
    }

    [[nodiscard]] std::vector<std::vector<InputEvent>> getWanderingSession()
    {
        std::vector<std::vector<InputEvent>> roundEvents;

        const auto wander = [&roundEvents](std::int32_t fromStep,
            std::int32_t toStep)
        {
            for (std::int32_t step = fromStep; step < toStep; ++step)
            {
                roundEvents.push_back(
                    {PointerMoved{.position = {.x = step, .y = step}}});
            }
        };

        wander(0, 10);
        roundEvents.push_back(
            {PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = 9, .y = 9}}});
        wander(10, 13);
        roundEvents.push_back(
            {PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = 12, .y = 12}}});
        wander(13, 18);
        roundEvents.push_back({PointerScrolled{.vertical = -2}});

        return roundEvents;
    }

    constexpr antwika::time::Tick kWanderingTicks = 21;

    [[nodiscard]] std::size_t inputEventsIn(
        const std::vector<TickEvent> &events)
    {
        const InputEventCodec codec;

        std::size_t count = 0;
        for (const auto &event : events)
        {
            if (codec.getDecodedEvent(event.event).has_value())
            {
                ++count;
            }
        }
        return count;
    }

    struct RunResult final
    {
        SessionSummary summary;
        std::vector<TickEvent> recordedEvents;
    };

    [[nodiscard]] RunResult run(
        antwika::event::ITickEventSource &source,
        antwika::time::Tick maxTicks)
    {
        ::testing::NiceMock<MockLogger> logger;

        EventDispatcher plainDispatcher({});
        FakeSummarySink sink;
        TickEventRecorder recorder;
        StopSignal stopSignal;
        TickedEventDispatcher dispatcher(
            plainDispatcher, {sink, stopSignal, recorder});
        Engine engine(logger, dispatcher);
        EngineLoop loop(engine, dispatcher, source);

        loop.run(stopSignal, maxTicks);

        return RunResult{
            .summary = sink.getResult(), .recordedEvents = recorder.getEvents()};
    }
}

TEST(
    InputDeterminismTest,
    Replay_ReachesTheSameStateAsALiveRun)
{
    constexpr antwika::time::Tick kMaxTicks = 20;

    ReplaySource stopAtTwoSource(
        {TickEvent{
            .tick = 2,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(getScriptedSession());
    const InputEventCodec codec;
    LiveInputSource liveSource(stopAtTwoSource, backend, codec);

    const auto liveRun = run(liveSource, kMaxTicks);

    ASSERT_NE(liveRun.summary, SessionSummary{});
    ASSERT_FALSE(liveRun.recordedEvents.empty());

    const antwika::testing::ScratchFile file(
        "antwika-input-determinism.replay");
    antwika::replay::saveReplayFile(liveRun.recordedEvents, file.getString());
    auto loadedEvents = antwika::replay::getLoadReplayFile(file.getString());

    ReplaySource replaySource(std::move(loadedEvents));
    const auto replayedRun = run(replaySource, kMaxTicks);

    EXPECT_EQ(replayedRun.summary, liveRun.summary);
    EXPECT_EQ(replayedRun.recordedEvents, liveRun.recordedEvents);
}

TEST(InputDeterminismTest, Live_FoldsSomething)
{
    constexpr antwika::time::Tick kMaxTicks = 20;

    ReplaySource stopAtTwoSource(
        {TickEvent{
            .tick = 2,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(getScriptedSession());
    const InputEventCodec codec;
    LiveInputSource liveSource(stopAtTwoSource, backend, codec);

    const auto liveRun = run(liveSource, kMaxTicks);

    EXPECT_EQ(liveRun.summary.pointerPosition, (Position{.x = 200, .y = 10}));
    EXPECT_EQ(liveRun.summary.pressedKeys, (std::vector<std::string>{"W"}));
    EXPECT_EQ(liveRun.summary.clicks, 2U);
    EXPECT_EQ(liveRun.summary.scrollTotal, 3);
    EXPECT_FALSE(liveRun.summary.leftHeldAtEnd);
}

TEST(InputDeterminismTest, Recording_KeepsInputAndDropsTicks)
{
    constexpr antwika::time::Tick kMaxTicks = 20;

    ReplaySource stopAtTwoSource(
        {TickEvent{
            .tick = 2,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(getScriptedSession());
    const InputEventCodec codec;
    LiveInputSource liveSource(stopAtTwoSource, backend, codec);

    const auto liveRun = run(liveSource, kMaxTicks);

    const antwika::testing::ScratchFile file("antwika-input-recording.replay");
    antwika::replay::saveReplayFile(liveRun.recordedEvents, file.getString());
    const auto loadedEvents = antwika::replay::getLoadReplayFile(file.getString());

    std::size_t inputEvents = 0;
    for (const auto &event : loadedEvents)
    {
        EXPECT_NE(event.event.name, antwika::engine::events::kTick);

        if (codec.getDecodedEvent(event.event).has_value())
        {
            ++inputEvents;
        }
    }

    EXPECT_EQ(inputEvents, getScriptedSession().size());
}

TEST(InputDeterminismTest, IdleMotionGate_ChangesNothingTheAppFolds)
{
    constexpr antwika::time::Tick kMaxTicks = 30;

    const InputEventCodec codec;

    ReplaySource ungatedStopSource(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend ungatedBackend(getWanderingSession());
    LiveInputSource ungatedLive(ungatedStopSource, ungatedBackend, codec);

    const auto ungatedRun = run(ungatedLive, kMaxTicks);

    ASSERT_EQ(ungatedRun.summary.clicks, 1U);
    ASSERT_EQ(ungatedRun.summary.scrollTotal, -2);

    ReplaySource gatedStopSource(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend gatedBackend(getWanderingSession());
    LiveInputSource gatedLive(gatedStopSource, gatedBackend, codec);
    IdleMotionFilter gatedFilter(gatedLive, codec);

    const auto recordedRun = run(gatedFilter, kMaxTicks);

    EXPECT_EQ(recordedRun.summary, ungatedRun.summary);
}

TEST(InputDeterminismTest, IdleMotionGate_KeepsOnlyMotionThatDidSomething)
{
    constexpr antwika::time::Tick kMaxTicks = 30;

    const InputEventCodec codec;
    ReplaySource stoppingSource(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(getWanderingSession());
    LiveInputSource liveSource(stoppingSource, backend, codec);
    IdleMotionFilter gatedFilter(liveSource, codec);

    const auto gatedRun = run(gatedFilter, kMaxTicks);

    EXPECT_EQ(inputEventsIn(gatedRun.recordedEvents), 8U);
}

TEST(InputDeterminismTest, Replay_ReachesTheSameStateWhenGated)
{
    constexpr antwika::time::Tick kMaxTicks = 30;

    const InputEventCodec codec;
    ReplaySource stoppingSource(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(getWanderingSession());
    LiveInputSource liveSource(stoppingSource, backend, codec);
    IdleMotionFilter gatedFilter(liveSource, codec);

    const auto liveRun = run(gatedFilter, kMaxTicks);

    ASSERT_NE(liveRun.summary, SessionSummary{});
    ASSERT_FALSE(liveRun.recordedEvents.empty());

    const antwika::testing::ScratchFile file("antwika-input-gated.replay");
    antwika::replay::saveReplayFile(liveRun.recordedEvents, file.getString());
    auto loadedEvents = antwika::replay::getLoadReplayFile(file.getString());

    ReplaySource replaySource(std::move(loadedEvents));
    IdleMotionFilter replayedGateFilter(replaySource, codec);
    const auto replayedRun = run(replayedGateFilter, kMaxTicks);

    EXPECT_EQ(replayedRun.summary, liveRun.summary);
    EXPECT_EQ(replayedRun.recordedEvents, liveRun.recordedEvents);
}
