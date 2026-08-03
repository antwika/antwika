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
#include <antwika/synth/SynthError.hpp>
#include <antwika/synth/TriggerRequest.hpp>

namespace antwika::music_editor
{

    Playback::TrackVoices::TrackVoices(
        synth::SynthMixer &mixer,
        std::uint64_t &counter,
        std::vector<ActiveNote> &notes,
        const sequencer::FrameClock &clock)
        : mixer(mixer),
          counter(counter),
          notes(notes),
          frameNumerator(clock.framesPerTick().numerator()),
          frameDenominator(clock.framesPerTick().denominator())
    {
    }

    namespace
    {
        // Which tick's audio a score frame falls in.
        // Integer arithmetic, so every run and toolchain agrees.
        [[nodiscard]] time::Tick tickOfFrame(
            const FrameIndex frame,
            const std::int64_t numerator,
            const std::int64_t denominator) noexcept
        {
            return (frame * static_cast<FrameIndex>(denominator))
                   / static_cast<FrameIndex>(numerator);
        }
    } // namespace

    void Playback::TrackVoices::trigger(
        const pattern::Controls &value,
        const FrameIndex startFrame,
        const FrameCount frames)
    {
        // The note, its harmony and the echo of both, in one place.
        // The offline waveform render sounds notes through it too.
        // A note the synth refuses is silence, and lights nothing.
        if (!soundNote(
                mixer, preset, value, frames, startFrame, offset))
        {
            return;
        }

        ++counter;

        // What to light, and for which ticks.
        // The span rode in on the event's own controls.
        // Never absent: everything here came through NoteWords.
        // That reader writes both controls into every word.
        // value() is what throws on a caller that broke that.
        const auto begin = value.get(kSpanBegin).value();
        const auto length = value.get(kSpanLength).value();

        // The tick whose audio window the note begins in.
        // Lit from that very tick, so the light keeps up with the ear.
        const auto from =
            tickOfFrame(startFrame, frameNumerator, frameDenominator);

        const auto rings = tickOfFrame(
            startFrame + frames, frameNumerator, frameDenominator);

        // At least the one tick it begins on.
        const auto until = std::max(from + 1, rings + 1);

        // The excluded lines carry the statement's unwind edges.
        // One is push_back's reallocation, one the string's copy.
        // The last is the note temporary's own unwind pad.
        // Only a failed allocation would take any of them.
        // See docs/confirming-unreachable-branches.md, signature (a).
        notes.push_back(ActiveNote{ // GCOVR_EXCL_LINE
            .voice = voiceIndex,
            .chain = std::string(chain), // GCOVR_EXCL_LINE
            .begin = static_cast<std::size_t>(begin.approximate()),
            .length = static_cast<std::size_t>(length.approximate()),
            .from = from,
            .until = until}); // GCOVR_EXCL_LINE
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
          shape(std::move(desc)),
          tempo(shape.framesPerCycle)
    {
        lead = static_cast<FrameCount>(
            shape.clock.frameAtTick(shape.lead));

        const auto tickFrames = shape.clock.frameAtTick(1);
        const auto rate =
            static_cast<std::int64_t>(mixer.format().rate);

        // One tick's worth of wall time, for pace()'s wait.
        interval = std::chrono::milliseconds{tickFrames * 1000 / rate};

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
                .tempo = tempo,
                .lookahead = shape.lookahead}; // GCOVR_EXCL_LINE

            perVoice.push_back(
                // The excluded line is the aggregate's unwind pad.
                // Only a failed allocation would take it.
                // See docs/confirming-unreachable-branches.md.
                Line{ // GCOVR_EXCL_LINE
                    .sequencer = std::make_unique<sequencer::Sequencer>(
                        std::move(each)),
                    .voices = std::make_unique<TrackVoices>(
                        mixer, counter, active, shape.clock),
                    // Never advanced, rather than advanced just now.
                    // On the run's first tick that reads as up to date.
                    // There is no history for it to have missed.
                    // Any later and it is a voice that joins.
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

            // A note that has rung out lights nothing any more.
            std::erase_if(
                active,
                [this](const ActiveNote &note)
                { return note.until <= played; });

            grow(voices.size());

            for (std::size_t at = 0; at < voices.size(); ++at)
            {
                auto &line = perVoice[at];

                // Every note lands at the offset the pause left.
                // That is constant while the clock is running.
                line.voices->offset = pausedFrames;
                line.voices->preset = voices[at].preset;
                line.voices->voiceIndex = at;
                line.voices->chain = score.chainOf(at);

                // A sequencer that missed a tick slept through it.
                // It joins now rather than playing what it missed.
                if (line.advanced + 1 != played)
                {
                    line.sequencer->joinAt(played - 1);
                }

                // A pattern can parse and still refuse a window.
                // Slow factors whose product overflows a Cycle do.
                // The line falls silent rather than the run ending.
                // It is advanced regardless: the window went by.
                try
                {
                    line.sequencer->advance(
                        played, voices[at].playing, *line.voices);
                }
                // The excluded line is the handler's no-match edge.
                // Only some other exception type would take it.
                // See docs/confirming-unreachable-branches.md.
                catch (const pattern::PatternError &) // GCOVR_EXCL_LINE
                {
                }

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
    // A real device paces the loop by lagging behind the queue.
    // One that consumes when pumped -- null, offline -- never lags.
    // Unpaced, an unbounded run over it spins a core flat out.
    // So a device that kept up exactly is paced to the clock instead.
    void Playback::pace()
    {
        const auto ahead = queued - device.framesPlayed();

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

        // What is not sounding must not stay lit.
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
        std::vector<DocumentSpan> lit;

        for (const auto &note : active)
        {
            // Decided ahead of time, lit only once it sounds.
            // The other bound needs no check.
            // A rung-out note was pruned by the step that outlived it.
            if (played < note.from)
            {
                continue;
            }

            // Its index outlives an edit above it; its chain does not.
            // A line rewritten or moved drops its notes' lights.
            // Dropped rather than guessed at, per the spanIn() rule.
            if (score.chainOf(note.voice) != note.chain)
            {
                continue;
            }

            const auto span =
                score.spanIn(note.voice, note.begin, note.length);

            // With the chain equal, every word still maps.
            // A word lives inside one physical line's segment.
            // The excluded refusal is spanIn's contract, not ours.
            // It serves a caller that skipped the chain check above.
            // See docs/confirming-unreachable-branches.md.
            if (span.has_value()) // GCOVR_EXCL_LINE
            {
                lit.push_back(*span);
            }
        }

        return lit;
        // Only an unwind destroys lit at this brace.
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

    void Playback::setSpeed(const sequencer::Rational speed)
    {
        // Twice as fast is half the frames to a cycle.
        const auto pace = shape.framesPerCycle / speed;

        // The next whole cycle no voice has been asked past.
        // Never inside a queried window: those notes' frames are out.
        // perVoice is never empty; the constructor grew the first line.
        auto asked = perVoice.front().sequencer->queriedThrough();

        for (const auto &line : perVoice)
        {
            asked = std::max(asked, line.sequencer->queriedThrough());
        }

        auto from = asked.sam() == asked ? asked : asked.nextSam();

        // Strictly after the last change, whose first is cycle zero.
        // So a second change inside one cycle lands a cycle later.
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

} // namespace antwika::music_editor
