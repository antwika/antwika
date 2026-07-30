#pragma once

#include <stdexcept>

namespace antwika::holdem
{

    /**
     * @brief Thrown by HandEvaluator when the cards handed to it could
     * never have come off a deck: too few or too many of them, the same
     * card twice, or a Card cast from an out-of-range integer.
     */
    class HandEvaluationError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::holdem
