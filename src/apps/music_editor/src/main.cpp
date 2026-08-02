#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/SelectedSoundBackend.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/MusicEditor.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/RenderSink.hpp"
#include "antwika/music_editor/TickBudgetSource.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::music_editor::EditorScene;
using antwika::music_editor::EditorSink;
using antwika::music_editor::PlaybackDesc;
using antwika::music_editor::RenderSink;
using antwika::music_editor::TickBudgetSource;
using antwika::replay::ReplaySource;
using antwika::sequencer::FrameClock;
using antwika::sequencer::Rational;
using antwika::sequencer::TempoMap;
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
    constexpr std::chrono::milliseconds kTickInterval{25};

    constexpr antwika::sound::WaveFormat kFormat{
        .rate = 48000, .channels = 2};

    // Two seconds to the cycle, so a four-slot line is a half a slot.
    // Typing into it is heard as a change rather than as a blur.
    constexpr std::int64_t kFramesPerCycle = 2 * kFormat.rate;

    // Capped, rather than run until the window is closed.
    // The default null backend reports no close at all.
    // It is also the build every CI leg produces.
    // At the interval above this is about two minutes of editing.
    constexpr antwika::time::Tick kTickBudget = 4800;

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);

        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        const auto soundBackend =
            antwika::sound::makeSelectedSoundBackend(logger);

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

        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true});

        WindowInputSource windowed(input, *backend, window->id());
        TickBudgetSource source(windowed, kTickBudget);

        const auto summary = antwika::music_editor::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = codec,
            .scene = scene,
            .mixer = mixer,
            .device = *device,
            .sleeper = sleeper,
            .playback =
                PlaybackDesc{
                    .clock = FrameClock(kFormat.rate, kTickInterval),
                    .tempo = TempoMap(Rational(kFramesPerCycle)),
                    .lookahead = 6,
                    .lead = 4},
            .canvas = kWindowSize,
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const EditorSink &editor)
            {
                return std::make_unique<RenderSink>(
                    *window, scene, editor);
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
