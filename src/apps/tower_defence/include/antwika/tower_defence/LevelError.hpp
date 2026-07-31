#pragma once

#include <stdexcept>
#include <string>

namespace antwika::tower_defence
{

    /**
     * @brief Thrown when a level cannot be generated from a config.
     *
     * One exception type for one failure category: either the config
     * describes a grid no level can exist in, or the solver ran out of
     * attempts before it produced one.
     */
    class LevelError final : public std::runtime_error
    {
    public:
        /**
         * @brief Construct the error with a description.
         * @param message What could not be generated, and why.
         */
        explicit LevelError(const std::string &message)
            : std::runtime_error(message)
        {
        }
    };

} // namespace antwika::tower_defence
