#include <chrono>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/conformance/ConsoleContract.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTrip.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/gfx/Size.hpp>
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
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/MusicEditor.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/SnapshotStore.hpp"
#include "antwika/music_editor/StateDump.hpp"

using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::log::mocks::MockLogger;
using antwika::music_editor::bootstrap;
using antwika::music_editor::EditorScene;
using antwika::music_editor::EditorSummary;
using antwika::music_editor::MusicEditorWiring;
using antwika::music_editor::openingState;
using antwika::music_editor::PlaybackDesc;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::sound::WaveFormat kFormat{
        .rate = 48000, .channels = 2};

    constexpr antwika::gfx::Size kCanvas{.width = 1120, .height = 640};

    [[nodiscard]] PlaybackDesc pacing()
    {
        return PlaybackDesc{
            .clock = antwika::sequencer::FrameClock(
                kFormat.rate, std::chrono::milliseconds{40}),
            .framesPerCycle = antwika::sequencer::Rational(kFormat.rate),
            .lookahead = 3,
            .lead = 2};
    }

    [[nodiscard]] EditorSummary run(
        std::vector<TickEvent> events,
        const std::string &dumpPath,
        const bool loadEnabled,
        const Tick stopTick = kOpenTick + 3)
    {
        events.push_back(stopAt(stopTick));

        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;

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
        ReplaySource source(std::move(events));

        antwika::console::ConsolePicture picture(kCanvas);

        return bootstrap(
            MusicEditorWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .scene = scene,
                .mixer = mixer,
                .device = device,
                .sleeper = sleeper,
                .playback = pacing(),
                .canvas = kCanvas,
                .consoleOverlay = picture,
                .consoleLoadEnabled = loadEnabled,
                .stateDumpPath = dumpPath,
                .maxTicks = 100});
    }

    // Reads a dump file back, magic and version checked on the way.
    [[nodiscard]] antwika::music_editor::EditorDump readDump(
        const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::music_editor::kStateDumpMagic,
             .version = antwika::music_editor::kStateDumpVersion},
            "antwika music editor state dump document",
            antwika::music_editor::standardStateDumpMigrations);

        return antwika::music_editor::editorDumpFromJson(
            format.read(path).state);
    }

    // This application's half of the shared console contract.
    struct MusicEditorConsoleTraits
    {
        using Summary = EditorSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            return ::run(std::move(script), dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        // A refused load leaves the pane where it opened.
        // LoadStateRestoresTheSessionAndItsClock reads that in full.
        static void expectUntouched(const Summary &)
        {
        }

        static std::string scratchPrefix()
        {
            return "antwika_music_editor_console.";
        }
    };
} // namespace

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        MusicEditor, ConsoleContract, MusicEditorConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        MusicEditor,
        ConsoleSnapshotRoundTrip,
        MusicEditorConsoleTraits);

} // namespace antwika::console::conformance

// The console is on top, so a letter under it is the console's.
// The document underneath keeps every character it had.
TEST(ConsoleSinkTest, ALetterUnderTheOpenConsoleNeverReachesTheScore)
{
    const antwika::testing::ScratchFile file(
        "antwika_music_editor_console_gate.json");

    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "qq");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));
    typeText(events, codec, kOpenTick + 1, "dump_state");
    events.push_back(keyAt(codec, kOpenTick + 1, Key::Enter));

    const auto summary =
        run(std::move(events), file.path().string(), true);

    ASSERT_EQ(summary.console.size(), 4U);
    EXPECT_EQ(summary.console[0], "> qq");
    EXPECT_EQ(summary.console[1], "unknown command: qq");

    // The qq and both Enters landed in the console alone.
    // An ungated editor would have typed them into the pane.
    const auto dump = readDump(file.path().string());
    EXPECT_EQ(dump.editor.source, openingState().source);
}

TEST(ConsoleSinkTest, DumpStateWritesTheSessionAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_music_editor_console_dump.json");

    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "dump_state");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));

    const auto summary =
        run(std::move(events), file.path().string(), true);

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> dump_state",
            "dumped state to " + file.path().string()}));
    EXPECT_TRUE(std::filesystem::exists(file.path()));
}

// The whole round: a session dumped by one run, loaded by another.
TEST(ConsoleSinkTest, LoadStateRestoresTheSessionAndItsClock)
{
    const antwika::testing::ScratchFile file(
        "antwika_music_editor_console_load.json");

    // A session well away from the opening one, dumped by hand.
    // Tall enough that a scroll of one line survives the clamp.
    // The pane clamps the scroll on the first frame after a load.
    antwika::music_editor::EditorState state = openingState();
    state.source = "$: drum.n(\"0*4\")\n";

    for (int line = 0; line < 40; ++line)
    {
        state.source += "// filler\n";
    }

    state.cursor = 3;
    state.scroll = 1;
    state.speed = 3;
    state.paused = true;

    antwika::music_editor::Score score;
    score.read(state.source);

    antwika::sound::Waveform rendered;
    antwika::sound::OfflineDevice device(
        antwika::sound::DeviceDesc{
            .format = kFormat, .preferredBufferFrames = 256},
        rendered);

    antwika::synth::SynthMixer mixer(
        antwika::synth::SynthMixerDesc{.format = kFormat});

    device.start(mixer);

    antwika::time::fakes::FakeSleeper sleeper;

    antwika::music_editor::Playback playback(
        score, mixer, device, sleeper, pacing());

    for (int at = 0; at < 25; ++at)
    {
        playback.step(false);
    }

    antwika::music_editor::MusicSnapshotStore store(
        state, score, playback);
    store.dump(file.path().string(), {});

    // The loading run: load_state, then dump_state to look inside.
    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "load_state");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));
    typeText(events, codec, kOpenTick + 1, "dump_state");
    events.push_back(keyAt(codec, kOpenTick + 1, Key::Enter));

    const auto summary =
        run(std::move(events), file.path().string(), true);

    // The load replaced the history with the dump's empty one.
    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "loaded state from " + file.path().string(),
            "> dump_state",
            "dumped state to " + file.path().string()}));

    // The restored session was paused, so its clock stood still.
    EXPECT_EQ(summary.played, 25U);

    // What the second dump wrote is the restored session.
    const auto dump = readDump(file.path().string());
    EXPECT_EQ(dump.editor.source, state.source);
    EXPECT_EQ(dump.editor.cursor, 3U);
    EXPECT_EQ(dump.editor.scroll, 1U);
    EXPECT_EQ(dump.editor.speed, 3U);
    EXPECT_TRUE(dump.editor.paused);
    EXPECT_EQ(dump.playback.played, 25U);
}

// No overlay named means no console, not an invisible one.
TEST(ConsoleSinkTest, AnEditorWithNoConsoleIgnoresTheKeys)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

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

    ReplaySource source({keyAt(codec, 1, Key::Grave), stopAt(3)});

    const auto summary = bootstrap(
        MusicEditorWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .scene = scene,
            .mixer = mixer,
            .device = device,
            .sleeper = sleeper,
            .playback = pacing(),
            .canvas = kCanvas,
            .maxTicks = 100});

    EXPECT_TRUE(summary.console.empty());
}
