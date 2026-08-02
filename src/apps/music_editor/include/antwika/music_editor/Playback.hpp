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
        sequencer::TempoMap tempo;

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


    private:
        // Turns one voice's events into that voice's sound.
        class TrackVoices final : public sequencer::ISequencerSink
        {
        public:
            TrackVoices(synth::SynthMixer &mixer, std::uint64_t &counter);

            void trigger(
                const pattern::Controls &value,
                FrameIndex startFrame,
                FrameCount frames) override;

            /** @brief Where the device timeline sits against the score. */
            FrameIndex offset = 0;

            /**
             * @brief The sound to make, which the line decides afresh
             * on every keystroke.
             */
            TrackPreset preset{};

        private:
            synth::SynthMixer &mixer;
            std::uint64_t &counter;
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

        std::vector<Line> perVoice;
        std::size_t voicesSounding = 0;

        FrameCount lead = 0;
        FrameIndex queued = 0;
        FrameIndex pausedFrames = 0;
        time::Tick played = 0;
        std::uint64_t counter = 0;
    };

} // namespace antwika::music_editor
