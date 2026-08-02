#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/ISequencerSink.hpp>
#include <antwika/sequencer/Sequencer.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;

    /**
     * @brief How playback is set up.
     */
    struct PlaybackDesc
    {
        sequencer::FrameClock clock;

        /**
         * @brief How long one cycle lasts at normal speed.
         *
         * The rational rather than a ready TempoMap, because a speed
         * change is worked out against this base -- twice as fast is
         * half these frames a cycle -- and a map alone has forgotten
         * what normal was.
         */
        sequencer::Rational framesPerCycle{};

        /**
         * @brief How many ticks ahead each track is decided.
         *
         * Must exceed the audio lead below, or a note would be decided
         * after the frames it belonged to had already been rendered.
         */
        time::Tick lookahead = 3;

        /** @brief How many ticks of audio to keep queued ahead. */
        time::Tick lead = 2;
    };

    /**
     * @brief Keeps every voice sounding, and keeps up with the score
     * changing under them.
     *
     * One sequencer per voice line, because a line's events have to be
     * made into a sound through *its* preset, and the sequencer's seam
     * hands on controls rather than sounds.
     *
     * **The count is the document's, not this class's.**
     * Sequencers are made as lines are written and kept when lines are
     * deleted, since a pool that only ever grows costs a few hundred
     * bytes and spares the run an allocation in the middle of a bar.
     * One taken up again -- because a line was written where a deleted
     * one used to be -- joins at the current tick rather than playing
     * the history it slept through.
     *
     * **Playing is the resting state.** Nothing starts it, and pausing
     * stops the musical clock rather than the device -- so a held note
     * rings out, the device never starves, and the frames that went by
     * while paused are counted so that resuming does not decide notes
     * for a moment already rendered.
     *
     * **The run is paced by how much audio the device has taken**,
     * which is the one thing `IDevice::framesPlayed()` is allowed to
     * decide.
     * A device that consumes the moment it is pumped is never ahead, so
     * a null or offline run costs no wall-clock time at all, while a
     * real one is paced by the hardware rather than by a second clock
     * with its own opinion.
     */
    /**
     * @brief One note that is sounding or about to, and what to light.
     *
     * Ticks rather than frames, so whether it is lit is one integer
     * comparison against the musical clock -- which stands still while
     * paused, exactly as the notes do.
     */
    struct ActiveNote
    {
        /** @brief Which voices() index decided it. */
        std::size_t voice = 0;

        /** @brief Where its word starts in that voice's notation. */
        std::size_t begin = 0;

        /** @brief How many characters the word runs for. */
        std::size_t length = 0;

        /** @brief The first tick it is lit on. */
        time::Tick from = 0;

        /** @brief One past the last tick it is lit on. */
        time::Tick until = 0;
    };

    class Playback final
    {
    public:
        /**
         * @brief Construct playback over everything it drives.
         * @param score What is playing; must outlive this object.
         * @param mixer What sounds it; must outlive this object.
         * @param device What it is pumped into; must outlive this.
         * @param sleeper Waits out whatever audio is already queued;
         * must outlive this object.
         * @param desc The two clocks and the two lookaheads.
         * @throws antwika::sequencer::SequencerError If a lookahead is
         * no ticks at all.
         */
        Playback(
            const Score &score,
            synth::SynthMixer &mixer,
            sound::IDevice &device,
            time::ISleeper &sleeper,
            PlaybackDesc desc);

        Playback(const Playback &) = delete;
        Playback(Playback &&) = delete;

        Playback &operator=(const Playback &) = delete;
        Playback &operator=(Playback &&) = delete;

        /**
         * @brief Decide one tick's notes and keep the device fed.
         * @param paused Whether the musical clock is standing still.
         */
        void step(bool paused);

        /**
         * @brief Get how many voices the last step sounded.
         * @return The count, which is the score's voice count.
         */
        [[nodiscard]] std::size_t sounding() const noexcept;

        /**
         * @brief Get where the notes sounding right now sit in the
         * document.
         *
         * **Tick-derived and deterministic**: a note's span rode in on
         * its own event's controls (see NoteWords), its active ticks
         * are integer arithmetic on where it falls in the score's
         * timeline, and the mapping onto the document is
         * Score::spanIn() against the text as it now stands -- so a
         * live run and its replay light the same characters, and
         * nothing here reads the device.  A span that no longer maps
         * -- its line deleted or rewritten -- is dropped rather than
         * guessed at.
         *
         * @return The spans, in the order their notes were decided.
         */
        [[nodiscard]] std::vector<DocumentSpan> highlights() const;

        /**
         * @brief Silence everything currently sounding.
         */
        void silence() noexcept;

        /**
         * @brief Get how many voices have been started in all.
         * @return The count.
         */
        [[nodiscard]] std::uint64_t started() const noexcept;

        /**
         * @brief Get how many voices are sounding right now.
         * @return The count.
         */
        [[nodiscard]] std::size_t voices() const noexcept;

        /**
         * @brief Get how many frames have been handed to the device.
         * @return The count.
         */
        [[nodiscard]] FrameIndex queuedFrames() const noexcept;

        /**
         * @brief Get how many ticks the musical clock has advanced.
         * @return The count, which stands still while paused.
         */
        [[nodiscard]] time::Tick playedTicks() const noexcept;

        /**
         * @brief Change how fast musical time runs, for every voice.
         *
         * Takes effect at the next whole cycle no voice has been asked
         * past, worked out in exact rational arithmetic -- so a live
         * run and its replay change pace at the very same note.  Notes
         * already decided keep the frames they were given.  A second
         * change inside one cycle lands a cycle after the first, since
         * a tempo boundary is a cycle's to hold and each cycle holds
         * one.
         *
         * @param speed The multiplier over the desc's base: 2 is twice
         * as fast, and 1/2 half.
         * @throws antwika::pattern::PatternError If the speed is zero
         * or the exact arithmetic will not fit.
         * @throws sequencer::SequencerError If the pace it works out
         * to would give a cycle no frames at all.
         */
        void setSpeed(sequencer::Rational speed);


    private:
        // Turns one voice's events into that voice's sound.
        class TrackVoices final : public sequencer::ISequencerSink
        {
        public:
            TrackVoices(
                synth::SynthMixer &mixer,
                std::uint64_t &counter,
                std::vector<ActiveNote> &notes,
                const sequencer::FrameClock &clock);

            void trigger(
                const pattern::Controls &value,
                FrameIndex startFrame,
                FrameCount frames) override;

            /** @brief Where the device timeline sits against the score. */
            FrameIndex offset = 0;

            /** @brief Which voices() index this line sounds as. */
            std::size_t voiceIndex = 0;

            /**
             * @brief The sound to make, which the line decides afresh
             * on every keystroke.
             */
            TrackPreset preset{};

        private:
            synth::SynthMixer &mixer;
            std::uint64_t &counter;

            // The one pool, owned by Playback, shared by every line.
            std::vector<ActiveNote> &notes;

            // Frames to ticks, as two integers.
            // The same exact arithmetic on every run and toolchain.
            std::int64_t frameNumerator;
            std::int64_t frameDenominator;
        };

        // One sequencer, its sink, and when it last ran.
        struct Line
        {
            std::unique_ptr<sequencer::Sequencer> sequencer;
            std::unique_ptr<TrackVoices> voices;
            time::Tick advanced = 0;
        };

        void grow(std::size_t count);

        void pump(bool paused);

        void pace();

        const Score &score;
        synth::SynthMixer &mixer;
        sound::IDevice &device;
        time::ISleeper &sleeper;

        PlaybackDesc shape;

        // The one timeline every voice runs on, speed changes and all.
        // A voice made later copies it, so a joiner keeps the pace.
        sequencer::TempoMap tempo;

        // Where the last speed change landed.
        // The next one must land strictly after it.
        sequencer::Rational retimed{};

        std::vector<Line> perVoice;
        std::size_t voicesSounding = 0;

        // Every note decided and not yet rung out, in decision order.
        std::vector<ActiveNote> active;

        FrameCount lead = 0;
        FrameIndex queued = 0;
        FrameIndex pausedFrames = 0;
        time::Tick played = 0;
        std::uint64_t counter = 0;
    };

} // namespace antwika::music_editor
