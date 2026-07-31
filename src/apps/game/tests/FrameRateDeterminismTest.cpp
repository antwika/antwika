#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <vector>

#include <antwika/app/FramePacedSource.hpp>
#include <antwika/app/fakes/FakeFramePass.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/PathIndex.hpp"

using antwika::app::FramePacedSource;
using antwika::app::fakes::FakeFramePass;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kMaxTicks = 40;
    constexpr std::chrono::milliseconds kInterval{40};

    [[nodiscard]] Event pressAt(Cell cell, MouseButton button)
    {
        const InputEventCodec codec;
        const auto point = cellCentre(cell, Camera());

        return codec.encode(
            PointerButtonPressed{
                .button = button,
                .position = {.x = point.x, .y = point.y}});
    }

    // A corridor and two walkers.
    // So the run has something that moves between two ticks.
    [[nodiscard]] std::vector<TickEvent> scriptedSession()
    {
        std::vector<TickEvent> events;

        for (std::int32_t x = 1; x <= 6; ++x)
        {
            events.push_back(
                TickEvent{
                    .tick = 0,
                    .event = pressAt(
                        Cell{.x = x, .y = 2}, MouseButton::Left)});
        }

        events.push_back(
            TickEvent{
                .tick = 1,
                .event =
                    pressAt(Cell{.x = 1, .y = 2}, MouseButton::Right)});
        events.push_back(
            TickEvent{
                .tick = 1,
                .event =
                    pressAt(Cell{.x = 6, .y = 2}, MouseButton::Right)});

        events.push_back(
            TickEvent{
                .tick = 12,
                .event = Event{.name = antwika::engine::events::kStop}});

        return events;
    }

    struct RunResult
    {
        GameSummary summary;
        std::vector<TickEvent> recorded;
        std::size_t frames = 0;
    };

    [[nodiscard]] RunResult runAt(std::uint32_t framesPerTick)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        TickEventRecorder recorder;
        AppModeState mode{AppMode::CityMap};

        auto script = scriptedSession();
        antwika::replay::ReplaySource inner(script);

        FakeFramePass pass;
        FakeSleeper sleeper;

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = kInterval, .framesPerTick = framesPerTick});

        auto summary = antwika::game::bootstrap(
            antwika::game::GameConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = paced,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .mode = mode,
                .maxTicks = kMaxTicks,
                .replayRecorder = recorder});

        return RunResult{
            .summary = std::move(summary),
            .recorded = recorder.getEvents(),
            .frames = pass.count()};
    }
} // namespace

// The property the whole frame-pacing design rests on.
// Drawing more often may change how a run looks.
// It must never change what a run computes.
TEST(FrameRateDeterminismTest, DrawingMoreOftenLeavesTheStateAlone)
{
    const auto sparse = runAt(1);
    const auto dense = runAt(7);

    EXPECT_EQ(sparse.summary, dense.summary);
}

TEST(FrameRateDeterminismTest, DrawingMoreOftenLeavesTheRecordingAlone)
{
    // A --record run writes the same file whatever it drew at.
    // Otherwise a session would not replay on another machine.
    const auto sparse = runAt(1);
    const auto dense = runAt(7);

    EXPECT_EQ(sparse.recorded, dense.recorded);
}

TEST(FrameRateDeterminismTest, TheDenserRunReallyDrewMore)
{
    // Two runs that both drew nothing would agree for a wrong reason.
    const auto sparse = runAt(1);
    const auto dense = runAt(7);

    EXPECT_EQ(sparse.frames, 0U);
    EXPECT_GT(dense.frames, sparse.frames);
}

TEST(FrameRateDeterminismTest, TheWholeRunTakesAsLongWhicheverRateItDrewAt)
{
    // Frames come out of the tick's interval, not on top of it.
    // So a smoother run is not a slower one.
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    Camera camera;
    PathIndex paths;
    TickEventRecorder recorder;
    AppModeState mode{AppMode::CityMap};

    auto script = scriptedSession();
    antwika::replay::ReplaySource inner(script);

    FakeFramePass pass;
    FakeSleeper sleeper;

    FramePacedSource paced(
        inner, pass, sleeper, {.tickInterval = kInterval, .framesPerTick = 7});

    const auto summary = antwika::game::bootstrap(
        antwika::game::GameConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = paced,
            .codec = codec,
            .extent = kExtent,
            .camera = camera,
            .paths = paths,
            .mode = mode,
            .maxTicks = kMaxTicks,
            .replayRecorder = recorder});

    // One interval per tick.
    // The source is asked for a tick's events exactly once.
    // And spends the whole interval before answering.
    EXPECT_EQ(sleeper.total(), kInterval * summary.state.ticksProcessed);
}
