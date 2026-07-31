#pragma once

#include <stdexcept>

namespace antwika::cli
{

    /**
     * @brief Thrown when a command line names a flag no program knows,
     * or leaves a flag that takes a value without one.
     *
     * Its own type rather than a bool, because the two cases read
     * identically at a call site: what was typed is not what this
     * program accepts, and neither case is anything the program's own
     * work went wrong at.
     */
    class CommandLineError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::cli
