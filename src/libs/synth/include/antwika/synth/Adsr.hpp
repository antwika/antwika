#pragma once

#include <antwika/sound/Frames.hpp>

namespace antwika::synth
{

    using antwika::sound::FrameCount;

    /**
     * @brief An amplitude envelope, in frames.
     *
     * **In frame time rather than musical time**, which is the
     * distinction the whole design rests on: this is projection state,
     * evaluated inside a voice, and it is not the thing a score would
     * modulate a parameter with.
     * Its counts are frames rather than seconds so that nothing on the
     * render path divides by a rate it would have to be told.
     */
    struct Adsr
    {
        /** @brief Frames spent climbing from silence to full. */
        FrameCount attack = 0;

        /** @brief Frames spent falling from full to the sustain level. */
        FrameCount decay = 0;

        /** @brief The level held after the decay, from zero to one. */
        float sustain = 1.0F;

        /** @brief Frames spent falling from the held level to silence. */
        FrameCount release = 0;

        /**
         * @brief Compare two envelopes.
         * @param other The envelope to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Adsr &other) const = default;
    };

    /**
     * @brief Get an envelope's amplitude at a moment.
     *
     * A pure function of how far a voice has got rather than a stage
     * machine something advances, for exactly the reason
     * antwika::animation has no `Animator` and antwika::tween's `ease` is
     * a pure function: a stage held between calls is state that cannot be
     * asked about out of order and cannot be asserted on without being
     * driven there first.
     *
     * The release always begins at `hold`, whatever the attack and decay
     * were doing when it arrived, and it falls from whatever level had
     * been reached.
     * So an envelope whose attack outlasts its hold is not an error, it
     * is a voice cut off while still rising -- which is what a short
     * percussive effect actually is.
     *
     * @param envelope The shape to evaluate.
     * @param since How many frames the voice has sounded for.
     * @param hold How many frames pass before the release begins.
     * @return The amplitude, from zero to one, and exactly zero once
     * `since` reaches `hold` plus the release.
     */
    [[nodiscard]] float envelopeAt(
        const Adsr &envelope, FrameCount since, FrameCount hold) noexcept;

} // namespace antwika::synth
