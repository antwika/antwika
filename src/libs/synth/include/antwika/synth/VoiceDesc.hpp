#pragma once

#include <cstdint>

#include <antwika/sound/Frames.hpp>

#include "antwika/synth/Adsr.hpp"
#include "antwika/synth/Filter.hpp"
#include "antwika/synth/Waveshape.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameCount;

    /**
     * @brief One sound, written down.
     *
     * **This is the type the whole library exists for.**
     * It is what replaces a `.wav` in the repository: a plain value a
     * person writes in source, reviews in a diff and merges in a branch,
     * rather than bytes exported from a tool nobody else has installed.
     *
     * It is also the reason there is no separate sound-effect subsystem.
     * The parameter set an sfxr-style effect needs -- a shape, an
     * envelope, a frequency and a slide -- **is** one synthesised voice,
     * and a note of music is one too.
     * So an explosion and a bassline are two of these, differing in their
     * numbers and in what decided to fire them, and they meet the same
     * pool through the same trigger.
     *
     * Every field is a plain number with no unit type around it, because
     * none of this is simulation state: a sample never reaches a tick,
     * and audio is a write-only projection in exactly rendering's sense.
     */
    struct VoiceDesc
    {
        /** @brief Which shape the oscillator traces. */
        Waveshape shape = Waveshape::Sine;

        /** @brief Where it starts, in hertz; ignored by Noise. */
        double frequency = 440.0;

        /**
         * @brief How fast the pitch moves, in hertz per second.
         *
         * The one thing separating a beep from an effect, and it is why
         * this field exists at all: a falling sweep is a hit landing, and
         * a rising one is a pickup.
         */
        double frequencySlide = 0.0;

        /** @brief The amplitude shape. */
        Adsr envelope{};

        /** @brief How many frames before the envelope's release begins. */
        FrameCount hold = 0;

        /** @brief What the oscillator is passed through. */
        FilterDesc filter{};

        /** @brief Linear, so one is unchanged and a half is quieter. */
        float gain = 1.0F;

        /** @brief Minus one is hard left, plus one hard right. */
        float pan = 0.0F;

        /** @brief What makes one noise voice differ from another. */
        std::uint64_t seed = 0;

        /**
         * @brief Get how many frames this voice sounds for in total.
         * @return The hold plus the release, which is when a voice
         * carrying it stops contributing.
         */
        [[nodiscard]] FrameCount totalFrames() const noexcept;

        /**
         * @brief Compare two descriptions.
         * @param other The description to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const VoiceDesc &other) const
            = default;
    };

} // namespace antwika::synth
