#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

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
     * @brief Keeps four lines sounding, and keeps up with them changing.
     *
     * One sequencer per track, because a track's events have to be made
     * into a voice through *its* preset, and the sequencer's seam hands
     * on controls rather than sounds.
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
        // Turns one track's events into that track's sound.
        class TrackVoices final : public sequencer::ISequencerSink
        {
        public:
            TrackVoices(
                const TrackPreset &preset,
                synth::SynthMixer &mixer,
                std::uint64_t &counter);

            void trigger(
                const pattern::Controls &value,
                FrameIndex startFrame,
                FrameCount frames) override;

            /** @brief Where the device timeline sits against the score. */
            FrameIndex offset = 0;

        private:
            const TrackPreset &preset;
            synth::SynthMixer &mixer;
            std::uint64_t &counter;
        };

        void pump(bool paused);

        void pace();

        const Score &score;
        synth::SynthMixer &mixer;
        sound::IDevice &device;
        time::ISleeper &sleeper;

        // Arrays rather than vectors.
        // The track count is a constant, so there is no growth path.
        std::array<std::unique_ptr<sequencer::Sequencer>, kTrackCount>
            sequencers;

        std::array<std::unique_ptr<TrackVoices>, kTrackCount> perTrack;

        FrameCount lead = 0;
        FrameIndex queued = 0;
        FrameIndex pausedFrames = 0;
        time::Tick played = 0;
        std::uint64_t counter = 0;
    };

} // namespace antwika::music_editor
