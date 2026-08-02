#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/synth/VoiceDesc.hpp>

namespace antwika::music_editor
{

    using antwika::pattern::Controls;
    using antwika::pattern::ParamId;
    using antwika::sound::FrameCount;
    using antwika::sound::SampleRate;
    using antwika::synth::VoiceDesc;

    /** @brief How many lines the editor plays at once. */
    inline constexpr std::size_t kTrackCount = 4;

    /**
     * @brief What a word in a line means.
     *
     * One id, because a line is a list of numbers and a number here is
     * a pitch.
     * antwika::pattern would carry any number of these; this app needs
     * one, and says so rather than inventing room it does not use.
     */
    inline constexpr ParamId kNote{1};

    /**
     * @brief The sound one line makes.
     *
     * **Written down here rather than sampled from anywhere**, which is
     * the whole point of the stack underneath: four presets are about
     * forty numbers in a header, and they are the entire instrument.
     *
     * Every duration is in milliseconds rather than frames, so a preset
     * says what it means at any rate and the conversion happens once,
     * where the rate is known.
     */
    struct TrackPreset
    {
        /** @brief What the oscillator traces. */
        synth::Waveshape shape = synth::Waveshape::Sine;

        /** @brief What a note of zero sounds at. */
        double baseHertz = 220.0;

        /** @brief How fast the pitch moves, in hertz per second. */
        double slide = 0.0;

        std::uint32_t attackMs = 0;
        std::uint32_t decayMs = 0;
        float sustain = 1.0F;
        std::uint32_t releaseMs = 60;

        /**
         * @brief The longest this preset holds, however long the note is.
         *
         * A drum is a hit whatever slot it lands in, and a bass note
         * fills its slot.
         * The difference between the two is this number.
         */
        std::uint32_t maxHoldMs = 2000;

        synth::FilterDesc filter{};

        float gain = 0.4F;
        float pan = 0.0F;

        /**
         * @brief Compare two presets.
         * @param other The preset to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const TrackPreset &other) const
            = default;
    };

    /**
     * @brief Get the four presets the editor plays.
     * @return The presets, indexed by track.
     */
    [[nodiscard]] const std::array<TrackPreset, kTrackCount> &
        trackPresets() noexcept;

    /**
     * @brief Turn one of a pattern's events into a voice.
     *
     * **The application's half of the sequencer's seam.**
     * antwika::sequencer says what begins and when in the caller's own
     * controls, and deciding that a control named kNote means a
     * semitone above a preset's base is exactly the decision it left
     * open.
     *
     * @param preset The line's sound.
     * @param value What the event carried.
     * @param frames How long the event lasts.
     * @param rate The rate the voice will run at.
     * @return The voice to trigger.
     */
    [[nodiscard]] VoiceDesc voiceFor(
        const TrackPreset &preset,
        const Controls &value,
        FrameCount frames,
        SampleRate rate);

} // namespace antwika::music_editor
