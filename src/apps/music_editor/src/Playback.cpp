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
        const TrackPreset &preset,
        synth::SynthMixer &mixer,
        std::uint64_t &counter)
        : preset(preset), mixer(mixer), counter(counter)
    {
    }

    void Playback::TrackVoices::trigger(
        const pattern::Controls &value,
        const FrameIndex startFrame,
        const FrameCount frames)
    {
        mixer.trigger(
            synth::TriggerRequest{
                .voice = voiceFor(
                    preset, value, frames, mixer.format().rate),
                .startFrame = startFrame + offset});

        ++counter;
    }

    Playback::Playback(
        const Score &score,
        synth::SynthMixer &mixer,
        sound::IDevice &device,
        time::ISleeper &sleeper,
        PlaybackDesc desc)
        : score(score), mixer(mixer), device(device), sleeper(sleeper)
    {
        lead = static_cast<FrameCount>(
            desc.clock.frameAtTick(desc.lead));

        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            sequencer::SequencerDesc each{
                .clock = desc.clock,
                .tempo = desc.tempo,
                .lookahead = desc.lookahead};

            sequencers[track] =
                std::make_unique<sequencer::Sequencer>(std::move(each));

            perTrack[track] = std::make_unique<TrackVoices>(
                trackPresets()[track], mixer, counter);
        }
    }

    void Playback::step(const bool paused)
    {
        if (!paused)
        {
            ++played;

            for (std::size_t track = 0; track < kTrackCount; ++track)
            {
                // Every note lands at the offset the pause left.
                // That is constant while the clock is running.
                perTrack[track]->offset = pausedFrames;

                sequencers[track]->advance(
                    played, score.playing(track), *perTrack[track]);
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
