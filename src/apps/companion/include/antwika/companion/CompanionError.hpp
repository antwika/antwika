#pragma once

#include <stdexcept>

namespace antwika::companion
{

    /**
     * @brief Thrown when a companion is asked for with numbers no run
     * could be balanced on, such as a need that never comes round.
     *
     * Its own type rather than a bare std::runtime_error, following the
     * one-exception-type-per-failure-category rule the rest of the
     * project follows.
     */
    class CompanionError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::companion
