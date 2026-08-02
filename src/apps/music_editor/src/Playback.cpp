#include "antwika/music_editor/Playback.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>

#include <antwika/pattern/Controls.hpp>
#include <antwika/sequencer/Sequencer.hpp>
#include <antwika/synth/TriggerRequest.hpp>

namespace antwika::music_editor
{

    Playback::TrackVoices::TrackVoices(
        synth::SynthMixer &mixer, std::uint64_t &counter)
        : mixer(mixer), counter(counter)
    {
    }

    void Playback::TrackVoices::trigger(
        const pattern::Controls &value,
        const FrameIndex startFrame,
        const FrameCount frames)
    {
        mixer.trigger(
            synth::TriggerRequest{
                // Seeded from where the note falls in the score.
                // Not from where the device is.
                // So a pause changes no hit's sound.
                .voice = voiceFor(
                    preset,
                    value,
                    frames,
                    mixer.format().rate,
                    startFrame),
                .startFrame = startFrame + offset});

        ++counter;
    }

    Playback::Playback(
        const Score &score,
        synth::SynthMixer &mixer,
        sound::IDevice &device,
        time::ISleeper &sleeper,
        PlaybackDesc desc)
        : score(score),
          mixer(mixer),
          device(device),
          sleeper(sleeper),
          // Held rather than read once.
          // A voice written later is made against the same shape.
          shape(std::move(desc))
    {
        lead = static_cast<FrameCount>(
            shape.clock.frameAtTick(shape.lead));

        // One, so a lookahead of no ticks is refused here.
        // Rather than on whichever keystroke first writes a voice.
        grow(1);
    }

    void Playback::grow(const std::size_t count)
    {
        while (perVoice.size() < count)
        {
            // The excluded line carries a (throw) edge.
            // It also carries an unwind pad for the TempoMap copy.
            // Both are taken only if an allocation actually fails.
            // See docs/confirming-unreachable-branches.md, signature (a).
            sequencer::SequencerDesc each{
                .clock = shape.clock,
                .tempo = shape.tempo,
                .lookahead = shape.lookahead}; // GCOVR_EXCL_LINE

            perVoice.push_back(
                Line{
                    .sequencer = std::make_unique<sequencer::Sequencer>(
                        std::move(each)),
                    .voices = std::make_unique<TrackVoices>(
                        mixer, counter),
                    .advanced = played});
        }
    }

    void Playback::step(const bool paused)
    {
        const auto &voices = score.voices();

        voicesSounding = voices.size();

        if (!paused)
        {
            ++played;

            grow(voices.size());

            for (std::size_t at = 0; at < voices.size(); ++at)
            {
                auto &line = perVoice[at];

                // Every note lands at the offset the pause left.
                // That is constant while the clock is running.
                line.voices->offset = pausedFrames;
                line.voices->preset = voices[at].preset;

                // A sequencer that missed a tick slept through it.
                // It joins now rather than playing what it missed.
                if (line.advanced + 1 != played)
                {
                    line.sequencer->joinAt(played - 1);
                }

                line.sequencer->advance(
                    played, voices[at].playing, *line.voices);

                line.advanced = played;
            }
        }

        pump(paused);
        pace();
    }

    void Playback::pump(const bool paused)
    {
        const auto ahead = queued - device.framesPlayed();

        if (ahead >= lead)
        {
            return;
        }

        const auto rendered = device.pump(lead - ahead);

        queued += rendered;

        // Frames that went by while the clock stood still.
        // Counting them stops a resume landing in the past.
        if (paused)
        {
            pausedFrames += rendered;
        }
    }

    // The one thing framesPlayed() is allowed to decide.
    // How long to wait, and never what to compute.
    // A device that consumes when pumped is never ahead of itself.
    // So an offline or null run never sleeps at all.
    // A real one is paced by the hardware rather than by a clock.
    void Playback::pace()
    {
        const auto target = lead / 2;
        const auto ahead = queued - device.framesPlayed();

        if (ahead <= target)
        {
            return;
        }

        const auto spare = ahead - target;
        const auto rate = static_cast<FrameCount>(mixer.format().rate);

        sleeper.sleep(
            std::chrono::milliseconds{
                static_cast<std::int64_t>(spare * 1000 / rate)});
    }

    void Playback::silence() noexcept
    {
        mixer.stopAll();
    }

    std::size_t Playback::voices() const noexcept
    {
        return mixer.activeVoices();
    }

    std::size_t Playback::sounding() const noexcept
    {
        return voicesSounding;
    }

    std::uint64_t Playback::started() const noexcept
    {
        return counter;
    }

    FrameIndex Playback::queuedFrames() const noexcept
    {
        return queued;
    }

    time::Tick Playback::playedTicks() const noexcept
    {
        return played;
    }

} // namespace antwika::music_editor
