#pragma once

#include <cstdint>

namespace antwika::sound
{

    /**
     * @brief A frame's position, counted from when a device started.
     *
     * **Absolute rather than relative to a buffer**, which is the one
     * decision here that matters: it is what lets a sound be scheduled
     * for an exact moment, and what lets a queue be slipped underneath
     * a caller later without that caller changing.
     */
    using FrameIndex = std::uint64_t;

    /**
     * @brief A number of frames.
     *
     * The same quantity as FrameIndex in a different role, which is why
     * the two are declared together rather than in two headers.
     */
    using FrameCount = std::uint64_t;

} // namespace antwika::sound
