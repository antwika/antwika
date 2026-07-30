#pragma once

#include <stdexcept>

namespace antwika::replay
{

    /**
     * @brief Thrown when a command line names a flag no program knows,
     * or leaves a flag that takes a value without one.
     *
     * Its own type rather than a bool, because the two cases read
     * identically at a call site and neither is a replay format problem.
     */
    class CommandLineError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::replay
