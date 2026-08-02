#pragma once

#include <chrono>

#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor::tests
{

    /** @brief The canvas every test here lays out against. */
    inline constexpr gfx::Size kCanvas{.width = 960, .height = 420};

    /**
     * @brief A whole editor over an offline device, built in one line.
     *
     * Everything a sink needs and nothing that reaches hardware, so a
     * test drives the real classes with no window and no speaker.
     */
    struct EditorRig
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

        Playback playback{
            score,
            mixer,
            device,
            sleeper,
            PlaybackDesc{
                .clock = sequencer::FrameClock(
                    format.rate, std::chrono::milliseconds{40}),
                .tempo = sequencer::TempoMap(
                    sequencer::Rational(format.rate)),
                .lookahead = 3,
                .lead = 2}};

        EditorSink editor{
            state, score, playback, codec, scene, kCanvas};

        EditorRig()
        {
            device.start(mixer);
        }
    };

} // namespace antwika::music_editor::tests
