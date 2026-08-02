#pragma once

#include <chrono>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
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
#include <antwika/time/Tick.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor::tests
{

    /**
     * @brief The canvas every test here lays out against.
     *
     * The size main() asks its window for, since the code pane is drawn
     * at twice the glyph scale and laying out against anything smaller
     * would be laying out a window no run ever opens.
     */
    inline constexpr gfx::Size kCanvas{.width = 1120, .height = 640};

    /**
     * @brief A whole editor over an offline device, built in one line.
     *
     * Everything a sink needs and nothing that reaches hardware, so a
     * test drives the real classes with no window and no speaker.
     *
     * The document it opens with is openingState()'s, which is four
     * voice lines; a test wanting its own writes over `state.source`
     * before the first tick, since the score is re-read from there on
     * every one of them.
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

    /**
     * @brief Make the tick event itself.
     * @param when Which tick it is.
     * @return The event a sink reads as "the tick happened".
     */
    [[nodiscard]] inline event::TickEvent tickAt(const time::Tick when)
    {
        return event::TickEvent{
            .tick = when,
            .event = {.name = antwika::engine::events::kTick}};
    }

    /**
     * @brief Run a stretch of ticks through an editor.
     * @param rig What to tick.
     * @param first The first tick to hand it.
     * @param last One past the last.
     */
    inline void tickThrough(
        EditorRig &rig, const time::Tick first, const time::Tick last)
    {
        for (time::Tick when = first; when < last; ++when)
        {
            rig.editor.handle(tickAt(when));
        }
    }

} // namespace antwika::music_editor::tests
