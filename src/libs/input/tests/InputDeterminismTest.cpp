#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/EngineLoop.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>

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
using antwika::replay::EngineLoop;
using antwika::replay::ReplaySource;

namespace
{
    // Removes its backing file on scope exit.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string name)
            : path((std::filesystem::temp_directory_path() / name).string())
        {
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;

        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        ~ScratchFile()
        {
            // A destructor must not throw, so use the non-throwing form.
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        [[nodiscard]] const std::string &name() const noexcept
        {
            return path;
        }

    private:
        std::string path;
    };

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

    struct RunResult
    {
        SessionSummary summary;
        std::vector<TickEvent> recorded;
    };

    // Runs the loop over whichever source it is handed.
    // Live and replayed differ only in that argument.
    // That is the whole claim under test.
    [[nodiscard]] RunResult run(
        antwika::replay::IReplaySource &source,
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
    const ScratchFile file("antwika-input-determinism.replay");
    antwika::replay::saveReplayFile(liveRun.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

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

    const ScratchFile file("antwika-input-recording.replay");
    antwika::replay::saveReplayFile(liveRun.recorded, file.name());
    const auto loaded = antwika::replay::loadReplayFile(file.name());

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
