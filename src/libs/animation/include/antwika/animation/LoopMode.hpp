#pragma once

#include <cstdint>

namespace antwika::animation
{

    /**
     * @brief What a clip does once its last frame has been shown.
     */
    enum class LoopMode : std::uint8_t
    {
        /**
         * @brief Start again from the first frame, forever.
         */
        Loop = 0,

        /**
         * @brief Hold the last frame, and report the clip as finished.
         */
        Once,
    };

} // namespace antwika::animation
