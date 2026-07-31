#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveformId.hpp"

namespace antwika::sound
{

    /**
     * @brief One sound, asked for at one moment.
     *
     * **startFrame is absolute and is never "now"**, which is the one
     * decision here worth defending: it is what makes placement
     * sample-accurate, and what would let a queue be slipped between a
     * caller and the mixer later without one caller changing.
     */
    struct PlayRequest
    {
        WaveformId waveform{};

        /** @brief The absolute frame its first sample lands on. */
        FrameIndex startFrame = 0;

        /** @brief Linear, so one is unchanged and a half is quieter. */
        float gain = 1.0F;

        /** @brief Minus one is hard left, plus one hard right. */
        float pan = 0.0F;

        /** @brief Whether it starts again when it runs out. */
        bool looping = false;

        /**
         * @brief Compare two requests.
         * @param other The request to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const PlayRequest &other) const
            = default;
    };

} // namespace antwika::sound
