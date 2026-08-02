#include "antwika/synth/Adsr.hpp"

#include <antwika/sound/Frames.hpp>

namespace antwika::synth
{

    namespace
    {
        // The level the attack and decay have reached by a frame.
        // The release needs to know what it falls *from*.
        // That is this same question, asked when the release began.
        [[nodiscard]] float beforeRelease(
            const Adsr &envelope, FrameCount since) noexcept
        {
            if (since < envelope.attack)
            {
                return static_cast<float>(since)
                    / static_cast<float>(envelope.attack);
            }

            const auto intoDecay = since - envelope.attack;

            if (intoDecay < envelope.decay)
            {
                const auto fallen = static_cast<float>(intoDecay)
                    / static_cast<float>(envelope.decay);

                return 1.0F - fallen * (1.0F - envelope.sustain);
            }

            return envelope.sustain;
        }
    } // namespace

    float envelopeAt(
        const Adsr &envelope, FrameCount since, FrameCount hold) noexcept
    {
        if (since < hold)
        {
            return beforeRelease(envelope, since);
        }

        const auto intoRelease = since - hold;

        if (intoRelease >= envelope.release)
        {
            // Exactly zero rather than nearly zero.
            // A residue here is a tail that never quite ends.
            // A voice is finished when its envelope says it is.
            return 0.0F;
        }

        const auto fallen = static_cast<float>(intoRelease)
            / static_cast<float>(envelope.release);

        return beforeRelease(envelope, hold) * (1.0F - fallen);
    }

} // namespace antwika::synth
