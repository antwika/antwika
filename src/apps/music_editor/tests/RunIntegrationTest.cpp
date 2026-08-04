#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/MusicEditor.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "EditorRig.hpp"

using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::log::mocks::MockLogger;
using antwika::music_editor::bootstrap;
using antwika::music_editor::EditorScene;
using antwika::music_editor::MusicEditorWiring;
using antwika::music_editor::PlaybackDesc;
using antwika::music_editor::tests::kCanvas;
using antwika::replay::ReplaySource;
using antwika::sound::WaveFormat;
using ::testing::NiceMock;

namespace
{
    constexpr WaveFormat kFormat{.rate = 48000, .channels = 2};
    constexpr antwika::time::Tick kBudget = 40;

    [[nodiscard]] PlaybackDesc pacing()
    {
        return PlaybackDesc{
            .clock = antwika::sequencer::FrameClock(
                kFormat.rate, std::chrono::milliseconds{40}),
            .framesPerCycle = antwika::sequencer::Rational(kFormat.rate),
            .lookahead = 3,
            .lead = 2};
    }

    // Counts the ticks it was handed, and draws nothing.
    class CountingSink final : public antwika::event::ITickEventSink
    {
    public:
        explicit CountingSink(int &counter) : counter(counter)
        {
        }

        void handle(const TickEvent &) override
        {
            ++counter;
        }

    private:
        int &counter;
    };

    // Refuses the first tick it is handed, mid-run.
    class ThrowingSink final : public antwika::event::ITickEventSink
    {
    public:
        void handle(const TickEvent &) override
        {
            throw std::runtime_error("the extra sink refused its tick");
        }
    };

    // The run has no budget of its own any more.
    // So every script here says when it is over, as a recording would.
    [[nodiscard]] TickEvent stopAt(const antwika::time::Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = antwika::event::Event{
                .name = antwika::engine::events::kStop}};
    }

    // Types into the opening document, then pauses with Escape.
    [[nodiscard]] std::vector<TickEvent> script(
        const antwika::time::Tick last)
    {
        const InputEventCodec codec;

        return {
            TickEvent{
                .tick = 2,
                .event = codec.encode(KeyPressed{.key = Key::Enter})},
            TickEvent{
                .tick = 3,
                .event = codec.encode(KeyPressed{.key = Key::Digit4,
                                                 .modifiers = {
                                                     .shift = true}})},
            TickEvent{
                .tick = 4,
                .event = codec.encode(KeyPressed{.key = Key::Backspace})},
            TickEvent{
                .tick = 5,
                .event = codec.encode(KeyPressed{.key = Key::Escape})},
            stopAt(last)};
    }
} // namespace

// The whole editor, wired as main() wires it.
// Over a device that never reaches a speaker.
TEST(RunIntegrationTest, RunsToItsScriptedStopAndSoundsTheOpeningDocument)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> events;

    antwika::sound::Waveform rendered;
    antwika::sound::OfflineDevice device(
        antwika::sound::DeviceDesc{
            .format = kFormat, .preferredBufferFrames = 256},
        rendered);

    antwika::synth::SynthMixer mixer(
        antwika::synth::SynthMixerDesc{.format = kFormat});

    device.start(mixer);

    antwika::time::fakes::FakeSleeper sleeper;
    const EditorScene scene;
    const InputEventCodec codec;

    ReplaySource source(script(kBudget));

    const auto summary = bootstrap(
        MusicEditorWiring{
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .scene = scene,
            .mixer = mixer,
            .device = device,
            .sleeper = sleeper,
            .playback = pacing(),
            .canvas = kCanvas,
            .maxTicks = kBudget + 8});

    // It played without anybody starting it, and drew every tick.
    EXPECT_GT(summary.notes, 0U);
    EXPECT_GT(summary.commands, 0U);
    EXPECT_GT(summary.reparses, 0U);

    // Escape arrived on tick five and paused it.
    // So the musical clock stopped well short of the budget.
    EXPECT_GT(summary.played, 0U);
    EXPECT_LT(summary.played, kBudget);
}

// A sink's refusal is not the run's to swallow.
// It unwinds the whole wiring rather than losing a tick silently.
TEST(RunIntegrationTest, ASinkThrowingMidRunUnwindsTheWholeWiring)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> events;

    antwika::sound::Waveform rendered;
    antwika::sound::OfflineDevice device(
        antwika::sound::DeviceDesc{
            .format = kFormat, .preferredBufferFrames = 256},
        rendered);

    antwika::synth::SynthMixer mixer(
        antwika::synth::SynthMixerDesc{.format = kFormat});

    device.start(mixer);

    antwika::time::fakes::FakeSleeper sleeper;
    const EditorScene scene;
    const InputEventCodec codec;

    ReplaySource source(script(kBudget));

    EXPECT_THROW(
        (void)bootstrap(
            MusicEditorWiring{
                .logger = logger,
                .eventSink = events,
                .inputSource = source,
                .codec = codec,
                .scene = scene,
                .mixer = mixer,
                .device = device,
                .sleeper = sleeper,
                .playback = pacing(),
                .canvas = kCanvas,
                .extraSink =
                    [](const antwika::music_editor::EditorSink &)
                {
                    return std::make_unique<ThrowingSink>();
                },
                .maxTicks = kBudget}),
        std::runtime_error);
}

// Nothing is drawn by the run itself.
// An extra sink is what a window would be attached through.
TEST(RunIntegrationTest, HandsTheEditorToWhateverElseWantsIt)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> events;

    antwika::sound::Waveform rendered;
    antwika::sound::OfflineDevice device(
        antwika::sound::DeviceDesc{
            .format = kFormat, .preferredBufferFrames = 256},
        rendered);

    antwika::synth::SynthMixer mixer(
        antwika::synth::SynthMixerDesc{.format = kFormat});

    device.start(mixer);

    antwika::time::fakes::FakeSleeper sleeper;
    const EditorScene scene;
    const InputEventCodec codec;

    ReplaySource source({stopAt(4)});

    int handed = 0;
    int ticks = 0;

    const auto summary = bootstrap(
        MusicEditorWiring{
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .scene = scene,
            .mixer = mixer,
            .device = device,
            .sleeper = sleeper,
            .playback = pacing(),
            .canvas = kCanvas,
            .extraSink =
                [&handed, &ticks](
                    const antwika::music_editor::EditorSink &)
                -> std::unique_ptr<antwika::event::ITickEventSink>
            {
                ++handed;

                return std::make_unique<CountingSink>(ticks);
            }});

    EXPECT_EQ(handed, 1);
    EXPECT_GT(ticks, 0);
    EXPECT_GT(summary.commands, 0U);
}

// A --record run puts the recorder in the same list.
// A session writes its file from the stream the editor read.
TEST(RunIntegrationTest, RecordsWhenItIsGivenSomewhereToRecord)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> events;

    antwika::sound::Waveform rendered;
    antwika::sound::OfflineDevice device(
        antwika::sound::DeviceDesc{
            .format = kFormat, .preferredBufferFrames = 256},
        rendered);

    antwika::synth::SynthMixer mixer(
        antwika::synth::SynthMixerDesc{.format = kFormat});

    device.start(mixer);

    antwika::time::fakes::FakeSleeper sleeper;
    const EditorScene scene;
    const InputEventCodec codec;

    ReplaySource source(script(8));

    antwika::event::TickEventRecorder recorder;

    const auto summary = bootstrap(
        MusicEditorWiring{
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .scene = scene,
            .mixer = mixer,
            .device = device,
            .sleeper = sleeper,
            .playback = pacing(),
            .canvas = kCanvas,
            .replayRecorder = recorder});

    EXPECT_GT(summary.commands, 0U);
    EXPECT_FALSE(recorder.getEvents().empty());
}
