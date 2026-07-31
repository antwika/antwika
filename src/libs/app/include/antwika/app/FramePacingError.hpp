#pragma once

#include <stdexcept>

namespace antwika::app
{

    /**
     * @brief Thrown when a frame pacing is not one a loop could honour,
     * such as a tick that draws no frames at all.
     *
     * Its own type rather than a bare std::runtime_error, following the
     * one-exception-type-per-failure-category rule the rest of the
     * project follows.
     */
    class FramePacingError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::app
