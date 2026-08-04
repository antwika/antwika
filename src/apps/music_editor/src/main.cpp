#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/FullscreenToggleSource.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowPointerMapping.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedClipboard.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/SelectedSoundBackend.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include <antwika/app/AssetPath.hpp>
#include "antwika/music_editor/ConfigFile.hpp"
#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/MusicEditor.hpp"
#include "antwika/music_editor/PasteSource.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/RenderSink.hpp"
#include "antwika/music_editor/ScoreFiles.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::music_editor::EditorScene;
using antwika::music_editor::EditorSink;
using antwika::music_editor::PasteSource;
using antwika::music_editor::PlaybackDesc;
using antwika::music_editor::RenderSink;
using antwika::replay::ReplaySource;
using antwika::sequencer::FrameClock;
using antwika::sequencer::Rational;
using antwika::simulation::WindowInputSource;
using antwika::time::SystemSleeper;

namespace
{
    // Twice the glyph and a document rather than four boxes.
    // Both want more window than a row of fields did.
    constexpr antwika::gfx::Size kWindowSize{
        .width = 1120, .height = 640};

    // A keystroke lands on a tick, and so does a note.
    // This is the granularity of both.

    constexpr antwika::sound::WaveFormat kFormat{
        .rate = 48000, .channels = 2};

    // One second to the cycle, a quarter of it to a four-slot beat.
    // Brisk enough to groove, and the speed box halves it from here.
    constexpr std::int64_t kFramesPerCycle = kFormat.rate;

    // The same key apps/game fills the screen with.
    // One editor with a different one would be one to remember.
    constexpr antwika::input::Key kFullscreenKey =
        antwika::input::Key::F10;

    // Where the menu's save writes and its load reads.
    // Beside the working directory, exactly as game's saves/ is.
    constexpr std::string_view kScoreDirectory{"scores"};

    void run(const RecordedRun &recorded)
    {
        // The numbers the run reads off config.json, once.
        const auto config =
            antwika::music_editor::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);

        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        const auto soundBackend =
            antwika::sound::makeSelectedSoundBackend(logger);

        const auto clipboard =
            antwika::input::makeSelectedClipboard(logger);

        logger.log(
            Level::Info,
            "Antwika music editor on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()) + ", sound: "
                + std::string(soundBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika music editor",
            .size = kWindowSize,
            .resizable = false});

        const auto device = soundBackend->openDevice(
            antwika::sound::DeviceDesc{.format = kFormat});

        antwika::synth::SynthMixer mixer(
            antwika::synth::SynthMixerDesc{.format = kFormat});

        device->start(mixer);

        const EditorScene scene;
        SystemSleeper sleeper;

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Where a window pixel is on the canvas, and nothing else.
        // Attached upstream of the recorder, exactly as apps/game.
        // So a file holds canvas positions and replays at any size.
        const antwika::app::WindowPointerMapping mapping(
            *window, kWindowSize);

        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .pointerMapping = mapping,
             .coalescePointerMotion = true});

        WindowInputSource windowed(input, *backend, window->id());

        // Above the loop, since filling the screen is not a tick's news.
        // It changes what the window reports its size to be.
        // It changes nothing this app lays out, hit-tests or plays.
        // The key press is ordinary recorded input all the same.
        // So a replay fills the screen where the run did.
        // And reaches the same state either way.
        antwika::app::FullscreenToggleSource fullscreen(
            windowed, *window, codec, kFullscreenKey);

        // A paste is external input, so it is read up here.
        // The recording carries the characters; a replay reads none.
        const bool live = !recorded.options.replayPath.has_value();

        PasteSource pasting(fullscreen, *clipboard, codec, live);

        // Until the window closes or a replay says stop, like game.
        // The null backend reports no close, so Ctrl+C ends one there.
        const auto summary = antwika::music_editor::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = pasting,
            .codec = codec,
            .scene = scene,
            .mixer = mixer,
            .device = *device,
            .sleeper = sleeper,
            .playback =
                PlaybackDesc{
                    .clock = FrameClock(
                        kFormat.rate,
                        std::chrono::milliseconds(
                            config.tickIntervalMs)),
                    .framesPerCycle = Rational(kFramesPerCycle),
                    .lookahead = 6,
                    .lead = 4},
            .canvas = kWindowSize,
            // A replay must not write this machine's clipboard either.
            .clipboard = live ? clipboard.get() : nullptr,
            .scoresDirectory = std::string(kScoreDirectory),
            // Nor this machine's scores; the state changes still do.
            .writesScores = live,
            // Once, before the loop; the list is the run's from here.
            .scores = antwika::music_editor::listScores(
                kScoreDirectory),
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const EditorSink &editor)
            {
                return std::make_unique<RenderSink>(
                    *window, scene, editor, kWindowSize);
            }});

        device->stop();

        logger.log(
            Level::Info,
            "Antwika music editor sounded "
                + std::to_string(summary.notes) + " notes over "
                + std::to_string(summary.played) + " played ticks");
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_music_editor", run);
}
