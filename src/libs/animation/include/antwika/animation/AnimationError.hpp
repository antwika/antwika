#pragma once

#include <stdexcept>

namespace antwika::animation
{

    /**
     * @brief Thrown when an animation value could not be built from the
     * numbers it was given.
     *
     * That covers a Clip with no frames, a frame whose duration is zero
     * ticks, a clip whose total duration would not fit in a
     * antwika::time::Tick, a Progress with a zero denominator or one
     * larger than its denominator, and a step of zero ticks.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::wfc::WfcError and antwika::replay::ReplayFormatError.
     * Every one of those is a construction-time contradiction rather
     * than a state a running animation can reach, so once a Clip exists
     * nothing in this library throws again.
     */
    class AnimationError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::animation
