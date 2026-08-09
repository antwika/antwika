#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <utility>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MemoryClipboard.hpp>
#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/WaveImage.hpp"

namespace antwika::music_editor::tests
{

    inline constexpr gfx::Size kCanvas{.width = 1120, .height = 640};

    struct EditorRig final
    {
        sound::WaveFormat format{.rate = 48000, .channels = 2};
        sound::Waveform rendered{};

        sound::OfflineDevice device{
            sound::DeviceDesc{
                .format = format, .preferredBufferFrames = 256},
            rendered};

        synth::SynthMixer mixer{
            synth::SynthMixerDesc{.format = format}};

        time::fakes::FakeSleeper sleeper{};

        EditorState state = openingState();
        Score score{};
        EditorScene scene{};
        input::InputEventCodec codec{};

        input::MemoryClipboard osClipboard{};

        engine::StopSignal stopSignal{};

        console::ConsolePicture consolePicture{kCanvas};

        std::string scoresDirectory;

        Playback playback{
            score,
            mixer,
            device,
            sleeper,
            PlaybackDesc{
                .clock = sequencer::FrameClock(
                    format.rate, std::chrono::milliseconds{40}),
                .framesPerCycle = sequencer::Rational(format.rate),
                .lookahead = 3,
                .lead = 2}};

        EditorSink editor;

        EditorRig() : EditorRig("scores")
        {
        }

        explicit EditorRig(std::string directory, bool writes = true)
            : scoresDirectory(std::move(directory)),
              editor{
                  state,
                  score,
                  playback,
                  codec,
                  scene,
                  kCanvas,
                  WaveRenderDesc{
                      .rate = format.rate,
                      .framesPerCycle =
                          sequencer::Rational(format.rate)},
                  std::ref(osClipboard),
                  stopSignal,
                  scoresDirectory,
                  writes}
        {
            device.start(mixer);
        }
    };

    [[nodiscard]] inline event::TickEvent tickAt(const time::Tick when)
    {
        return event::TickEvent{
            .tick = when,
            .event = {.name = antwika::engine::events::kTick}};
    }

    inline void tickThrough(
        EditorRig &rig, const time::Tick first, const time::Tick last)
    {
        for (time::Tick when = first; when < last; ++when)
        {
            rig.editor.handle(tickAt(when));
        }
    }

}
