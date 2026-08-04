#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <unistd.h>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/simulation/EngineLoop.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/input/IdleMotionSource.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"
#include "antwika/input/fakes/FakeInputBackend.hpp"

using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::EventDispatcher;
using antwika::event::ITickEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::event::TickedEventDispatcher;
using antwika::input::IdleMotionSource;
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
using antwika::log::mocks::MockLogger;
using antwika::simulation::EngineLoop;
using antwika::replay::ReplaySource;

namespace
{
    // What a session amounted to, in terms an application cares about.
    // Comparable, so two runs are asserted equal rather than hashed.
    // A hash would agree for the wrong reason if both folded nothing.
    struct SessionSummary
    {
        Position pointer;
        std::vector<std::string> pressedKeys;
        std::uint32_t clicks = 0;
        std::int32_t scrolled = 0;
        bool leftHeldAtEnd = false;

        [[nodiscard]] bool operator==(
            const SessionSummary &other) const = default;
    };

    // Folds input in the tick path, downstream of the recorder.
    // That is where translating input into app meaning belongs.
    class SummarySink final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override
        {
            if (event.event.name == antwika::engine::events::kTick)
            {
                state.beginTick();
                return;
            }

            const auto decoded = codec.decode(event.event);
            if (!decoded.has_value())
            {
                return;
            }

            state.apply(*decoded);

            if (const auto *pressed = std::get_if<KeyPressed>(&*decoded))
            {
                summary.pressedKeys.push_back(
                    std::string(toString(pressed->key)));
            }

            if (std::holds_alternative<PointerButtonPressed>(*decoded))
            {
                ++summary.clicks;
            }

            // Taken from the event, not from mouse().scroll().
            // That is a per-tick running total.
            // Adding it per event would count one notch many times.
            if (const auto *scroll =
                    std::get_if<PointerScrolled>(&*decoded))
            {
                summary.scrolled += scroll->vertical;
            }

            summary.pointer = state.mouse().position();
            summary.leftHeldAtEnd = state.mouse().isDown(MouseButton::Left);
        }

        [[nodiscard]] SessionSummary result() const
        {
            return summary;
        }

    private:
        InputEventCodec codec;
        InputState state;
        SessionSummary summary;
    };

    // The session a user would have had: move, click, drag, scroll, type.
    [[nodiscard]] std::vector<InputEvent> scriptedSession()
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

    // A session that is mostly the pointer crossing an empty window.
    // One round per tick, so the wandering really does span ticks:
    // ten ticks of it, a drag, five more, then a scroll.
    [[nodiscard]] std::vector<std::vector<InputEvent>> wanderingSession()
    {
        std::vector<std::vector<InputEvent>> rounds;

        const auto wander = [&rounds](std::int32_t from, std::int32_t to)
        {
            for (std::int32_t step = from; step < to; ++step)
            {
                rounds.push_back(
                    {PointerMoved{.position = {.x = step, .y = step}}});
            }
        };

        wander(0, 10);
        rounds.push_back(
            {PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = 9, .y = 9}}});
        wander(10, 13);
        rounds.push_back(
            {PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = 12, .y = 12}}});
        wander(13, 18);
        rounds.push_back({PointerScrolled{.vertical = -2}});

        return rounds;
    }

    // How many rounds the session above spans, and so when to stop.
    constexpr antwika::time::Tick kWanderingTicks = 21;

    [[nodiscard]] std::size_t inputEventsIn(
        const std::vector<TickEvent> &events)
    {
        const InputEventCodec codec;

        std::size_t count = 0;
        for (const auto &event : events)
        {
            if (codec.decode(event.event).has_value())
            {
                ++count;
            }
        }
        return count;
    }

    struct RunResult
    {
        SessionSummary summary;
        std::vector<TickEvent> recorded;
    };

    // Runs the loop over whichever source it is handed.
    // Live and replayed differ only in that argument.
    // That is the whole claim under test.
    [[nodiscard]] RunResult run(
        antwika::simulation::ITickEventSource &source,
        antwika::time::Tick maxTicks)
    {
        ::testing::NiceMock<MockLogger> logger;

        EventDispatcher plain({});
        SummarySink sink;
        TickEventRecorder recorder;
        StopSignal stopSignal;
        TickedEventDispatcher dispatcher(
            plain, {sink, stopSignal, recorder});
        Engine engine(logger, dispatcher);
        EngineLoop loop(engine, dispatcher, source);

        loop.run(stopSignal, maxTicks);

        return RunResult{
            .summary = sink.result(), .recorded = recorder.getEvents()};
    }
} // namespace

// The claim the whole library rests on.
// A session driven by a live device records to a replay.
// That replay then reproduces the same state.
TEST(
    InputDeterminismTest,
    ARecordedLiveSessionReplaysToTheSameState)
{
    constexpr antwika::time::Tick kMaxTicks = 20;

    // The stop has to be input too.
    // Otherwise the replay would not know when the live run ended.
    ReplaySource stopAtTwo(
        {TickEvent{
            .tick = 2,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(scriptedSession());
    const InputEventCodec codec;
    LiveInputSource live(stopAtTwo, backend, codec);

    const auto liveRun = run(live, kMaxTicks);

    // Round-trip through the real save and load.
    // That exercises the engine.tick filtering rather than assuming it.
    const antwika::testing::ScratchFile file(
        "antwika-input-determinism.replay");
    antwika::replay::saveReplayFile(liveRun.recorded, file.string());
    auto loaded = antwika::replay::loadReplayFile(file.string());

    ReplaySource replayed(std::move(loaded));
    const auto replayedRun = run(replayed, kMaxTicks);

    EXPECT_EQ(replayedRun.summary, liveRun.summary);
    EXPECT_EQ(replayedRun.recorded, liveRun.recorded);
}

TEST(InputDeterminismTest, ALiveSessionActuallyFoldsSomething)
{
    constexpr antwika::time::Tick kMaxTicks = 20;

    ReplaySource stopAtTwo(
        {TickEvent{
            .tick = 2,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(scriptedSession());
    const InputEventCodec codec;
    LiveInputSource live(stopAtTwo, backend, codec);

    const auto liveRun = run(live, kMaxTicks);

    // Two runs that both did nothing would agree for the wrong reason.
    EXPECT_EQ(liveRun.summary.pointer, (Position{.x = 200, .y = 10}));
    EXPECT_EQ(liveRun.summary.pressedKeys, (std::vector<std::string>{"W"}));
    EXPECT_EQ(liveRun.summary.clicks, 2U);
    EXPECT_EQ(liveRun.summary.scrolled, 3);
    EXPECT_FALSE(liveRun.summary.leftHeldAtEnd);
}

TEST(InputDeterminismTest, TheRecordingKeepsTheInputAndDropsTheTicks)
{
    constexpr antwika::time::Tick kMaxTicks = 20;

    ReplaySource stopAtTwo(
        {TickEvent{
            .tick = 2,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(scriptedSession());
    const InputEventCodec codec;
    LiveInputSource live(stopAtTwo, backend, codec);

    const auto liveRun = run(live, kMaxTicks);

    const antwika::testing::ScratchFile file("antwika-input-recording.replay");
    antwika::replay::saveReplayFile(liveRun.recorded, file.string());
    const auto loaded = antwika::replay::loadReplayFile(file.string());

    std::size_t inputEvents = 0;
    for (const auto &event : loaded)
    {
        EXPECT_NE(event.event.name, antwika::engine::events::kTick);

        if (codec.decode(event.event).has_value())
        {
            ++inputEvents;
        }
    }

    EXPECT_EQ(inputEvents, scriptedSession().size());
}

// What the idle-motion gate must never do: change the outcome.
// The argument for holding a movement back is that nothing reads it.
// So a run with the gate and one without must fold the same state.
TEST(InputDeterminismTest, TheIdleMotionGateChangesNothingTheAppFolds)
{
    constexpr antwika::time::Tick kMaxTicks = 30;

    const InputEventCodec codec;

    ReplaySource ungatedStop(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend ungatedBackend(wanderingSession());
    LiveInputSource ungatedLive(ungatedStop, ungatedBackend, codec);

    const auto ungated = run(ungatedLive, kMaxTicks);

    ReplaySource gatedStop(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend gatedBackend(wanderingSession());
    LiveInputSource gatedLive(gatedStop, gatedBackend, codec);
    IdleMotionSource gated(gatedLive, codec);

    const auto run_ = run(gated, kMaxTicks);

    EXPECT_EQ(run_.summary, ungated.summary);
}

TEST(InputDeterminismTest, TheIdleMotionGateKeepsOnlyMotionThatDidSomething)
{
    constexpr antwika::time::Tick kMaxTicks = 30;

    const InputEventCodec codec;
    ReplaySource stopping(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(wanderingSession());
    LiveInputSource live(stopping, backend, codec);
    IdleMotionSource gated(live, codec);

    const auto gatedRun = run(gated, kMaxTicks);

    // The press, three movements mid-drag, the release and the scroll.
    // Plus one held-back movement ahead of each of press and scroll.
    // The other thirteen were superseded before anything read them.
    EXPECT_EQ(inputEventsIn(gatedRun.recorded), 8U);
}

TEST(InputDeterminismTest, AGatedLiveSessionReplaysToTheSameState)
{
    constexpr antwika::time::Tick kMaxTicks = 30;

    const InputEventCodec codec;
    ReplaySource stopping(
        {TickEvent{
            .tick = kWanderingTicks,
            .event = {.name = antwika::engine::events::kStop}}});
    FakeInputBackend backend(wanderingSession());
    LiveInputSource live(stopping, backend, codec);
    IdleMotionSource gated(live, codec);

    const auto liveRun = run(gated, kMaxTicks);

    const antwika::testing::ScratchFile file("antwika-input-gated.replay");
    antwika::replay::saveReplayFile(liveRun.recorded, file.string());
    auto loaded = antwika::replay::loadReplayFile(file.string());

    // Gated on the way back too, since both paths run one pipeline.
    // An already gated stream must come through it unchanged.
    ReplaySource replayed(std::move(loaded));
    IdleMotionSource replayedGate(replayed, codec);
    const auto replayedRun = run(replayedGate, kMaxTicks);

    EXPECT_EQ(replayedRun.summary, liveRun.summary);
    EXPECT_EQ(replayedRun.recorded, liveRun.recorded);
}
