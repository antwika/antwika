#pragma once

#include <antwika/sound/Frames.hpp>

#include "antwika/synth/VoiceDesc.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameIndex;

    /**
     * @brief One voice, asked for at one moment.
     *
     * The counterpart of antwika::sound::PlayRequest, and deliberately
     * the same shape: **startFrame is absolute and is never "now"**.
     * That is what makes placement sample-accurate, and it is what lets a
     * sequencer decide on a tick that a note belongs three thousand
     * frames later and be believed.
     */
    struct TriggerRequest
    {
        /** @brief The sound to make. */
        VoiceDesc voice{};

        /** @brief The absolute frame its first sample lands on. */
        FrameIndex startFrame = 0;

        /**
         * @brief Compare two requests.
         * @param other The request to compare against.
         * @return True when the voice and the start frame both match.
         */
        [[nodiscard]] bool operator==(const TriggerRequest &other) const
            = default;
    };

} // namespace antwika::synth
