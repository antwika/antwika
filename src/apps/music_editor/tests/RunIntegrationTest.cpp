#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/mocks/MockTickEventSink.hpp>
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
using antwika::event::mocks::MockTickEventSink;
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
using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::Throw;

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

    [[nodiscard]] TickEvent stopAt(const antwika::time::Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = antwika::event::Event{
                .name = antwika::engine::events::kStop}};
    }

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
}

TEST(RunIntegrationTest, Run_SoundsTheOpeningDocumentToItsStop)
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

    EXPECT_GT(summary.notes, 0U);
    EXPECT_GT(summary.commands, 0U);
    EXPECT_GT(summary.reparses, 0U);

    EXPECT_GT(summary.played, 0U);
    EXPECT_LT(summary.played, kBudget);
}

TEST(RunIntegrationTest, Run_UnwindsTheWiringOnAThrow)
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
                    auto sink =
                        std::make_unique<NiceMock<MockTickEventSink>>();

                    ON_CALL(*sink, handle(_))
                        .WillByDefault(Throw(std::runtime_error(
                            "the extra sink refused its tick")));

                    return sink;
                },
                .maxTicks = kBudget}),
        std::runtime_error);
}

TEST(RunIntegrationTest, Run_HandsTheEditorToOtherWiring)
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
    auto extra = std::make_unique<NiceMock<MockTickEventSink>>();

    EXPECT_CALL(*extra, handle(_)).Times(AtLeast(1));

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
                [&handed, &extra](
                    const antwika::music_editor::EditorSink &)
                -> std::unique_ptr<antwika::event::ITickEventSink>
            {
                ++handed;

                return std::move(extra);
            }});

    EXPECT_EQ(handed, 1);
    EXPECT_GT(summary.commands, 0U);
}

TEST(RunIntegrationTest, Run_RecordsWhenGivenSomewhereToRecord)
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
