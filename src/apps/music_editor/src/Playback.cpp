#include "antwika/music_editor/Playback.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/sequencer/Sequencer.hpp>
#include <antwika/sequencer/SequencerError.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/synth/SynthError.hpp>
#include <antwika/synth/TriggerRequest.hpp>

#include "antwika/music_editor/StateDumpError.hpp"

namespace antwika::music_editor
{

    Playback::TrackVoices::TrackVoices(
        synth::SynthMixer &mixer,
        std::uint64_t &counter,
        std::vector<ActiveNote> &notes)
        : mixer(mixer), counter(counter), notes(notes)
    {
    }

    void Playback::TrackVoices::trigger(
        const pattern::Controls &value,
        const FrameIndex startFrame,
        const FrameCount frames)
    {
        if (!soundNote(
                mixer, preset, value, frames, startFrame, offset))
        {
            return;
        }

        ++counter;

        const auto begin = value.get(kSpanBegin).value();
        const auto length = value.get(kSpanLength).value();

        const auto begins = startFrame + offset;
        const auto lasts =
            soundingFrames(preset, frames, mixer.format().rate);

        notes.push_back(ActiveNote{ // GCOVR_EXCL_LINE
            .voice = voiceIndex,
            .chain = std::string(chain), // GCOVR_EXCL_LINE
            .begin = static_cast<std::size_t>(begin.approximate()),
            .length = static_cast<std::size_t>(length.approximate()),
            .beginFrame = begins,
            .endFrame = begins + lasts}); // GCOVR_EXCL_LINE
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
          shape(std::move(desc)),
          tempo(shape.framesPerCycle)
    {
        lead = static_cast<FrameCount>(
            shape.clock.frameAtTick(shape.lead));

        const auto tickFrames = shape.clock.frameAtTick(1);
        const auto rate =
            static_cast<std::int64_t>(mixer.format().rate);

        interval = std::chrono::milliseconds{tickFrames * 1000 / rate};

        grow(1);
    }

    void Playback::grow(const std::size_t count)
    {
        while (perVoice.size() < count)
        {
            sequencer::SequencerDesc each{
                .clock = shape.clock,
                .tempo = tempo,
                .lookahead = shape.lookahead}; // GCOVR_EXCL_LINE

            perVoice.push_back(
                Line{ // GCOVR_EXCL_LINE
                    .sequencer = std::make_unique<sequencer::Sequencer>(
                        std::move(each)),
                    .voices = std::make_unique<TrackVoices>(
                        mixer, counter, active),
                    .advanced = 0});
        }
    }

    void Playback::step(const bool paused)
    {
        const auto &voices = score.voices();

        voicesSounding = voices.size();

        if (!paused)
        {
            ++played;

            const auto sounded = soundedFrame();

            std::erase_if(
                active,
                [sounded](const ActiveNote &note)
                { return note.endFrame <= sounded; });

            grow(voices.size());

            for (std::size_t at = 0; at < voices.size(); ++at)
            {
                auto &line = perVoice[at];

                line.voices->offset = pausedFrames;
                line.voices->preset = voices[at].preset;
                line.voices->voiceIndex = at;
                line.voices->chain = score.chainOf(at);

                if (line.advanced + 1 != played)
                {
                    line.sequencer->joinAt(played - 1);
                }

                try
                {
                    line.sequencer->advance(
                        played, voices[at].playing, *line.voices);
                }
                catch (const pattern::PatternError &) // GCOVR_EXCL_LINE
                {
                }

                line.advanced = played;
            }
        }

        pump(paused);
        pace();
    }

    FrameIndex Playback::soundedFrame() const
    {
        return device.framesPlayed() + origin;
    }

    FrameCount Playback::backlog() const
    {
        const auto sounded = soundedFrame();

        return queued > sounded ? queued - sounded : 0;
    }

    void Playback::pump(const bool paused)
    {
        const auto ahead = backlog();

        if (ahead >= lead)
        {
            return;
        }

        const auto rendered = device.pump(lead - ahead);

        queued += rendered;

        if (paused)
        {
            pausedFrames += rendered;
        }
    }

    void Playback::pace()
    {
        const auto ahead = backlog();

        if (ahead == 0)
        {
            sleeper.sleep(interval);

            return;
        }

        const auto target = lead / 2;

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

        active.clear();
    }

    std::size_t Playback::voices() const noexcept
    {
        return mixer.activeVoices();
    }

    std::size_t Playback::sounding() const noexcept
    {
        return voicesSounding;
    }

    std::vector<DocumentSpan> Playback::highlights() const
    {
        const auto sounded = soundedFrame();

        std::vector<DocumentSpan> lit;

        for (const auto &note : active)
        {
            if (sounded < note.beginFrame)
            {
                continue;
            }

            if (score.chainOf(note.voice) != note.chain)
            {
                continue;
            }

            const auto span =
                score.spanIn(note.voice, note.begin, note.length);

            if (span.has_value()) // GCOVR_EXCL_LINE
            {
                lit.push_back(*span);
            }
        }

        return lit;
    } // GCOVR_EXCL_LINE

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

    sequencer::Rational Playback::position() const
    {
        return tempo.cycleAt(shape.clock.frameAtTick(played));
    }

    PlaybackMemory Playback::remember() const
    {
        return PlaybackMemory{ // GCOVR_EXCL_LINE
            .segments = tempo.segments(), // GCOVR_EXCL_LINE
            .retimed = retimed,
            .played = played,
            .counter = counter,
            .queued = queued,
            .pausedFrames = pausedFrames,
            .voiceCount = perVoice.size()}; // GCOVR_EXCL_LINE
    }

    void Playback::restore(const PlaybackMemory &memory)
    {
        if (memory.segments.empty())
        {
            throw StateDumpError(
                "antwika::music_editor: a dump remembers no tempo "
                "segment at all");
        }

        sequencer::TempoMap rebuilt(
            memory.segments.front().framesPerCycle);

        try
        {
            for (std::size_t at = 1; at < memory.segments.size(); ++at)
            {
                rebuilt.addSegment(
                    memory.segments[at].startCycle,
                    memory.segments[at].framesPerCycle);
            }
        }
        catch (const sequencer::SequencerError &refused)
        {
            throw StateDumpError(refused.what());
        }

        silence();

        tempo = std::move(rebuilt);
        retimed = memory.retimed;

        played = memory.played;
        counter = memory.counter;
        queued = memory.queued;
        pausedFrames = memory.pausedFrames;

        const auto sounded = device.framesPlayed();
        origin = queued > sounded ? queued - sounded : 0;
        active.clear();

        perVoice.clear();
        grow(std::max<std::size_t>(memory.voiceCount, 1));

        for (auto &line : perVoice)
        {
            line.sequencer->joinAt(played);
            line.advanced = played;
        }
    }

    void Playback::setSpeed(const sequencer::Rational speed)
    {
        const auto pace = shape.framesPerCycle / speed;

        auto asked = perVoice.front().sequencer->queriedThrough();

        for (const auto &line : perVoice)
        {
            asked = std::max(asked, line.sequencer->queriedThrough());
        }

        auto from = asked.sam() == asked ? asked : asked.nextSam();

        if (from <= retimed)
        {
            from = retimed + sequencer::Rational{1};
        }

        tempo.addSegment(from, pace);

        for (const auto &line : perVoice)
        {
            line.sequencer->retime(from, pace);
        }

        retimed = from;
    }

}
