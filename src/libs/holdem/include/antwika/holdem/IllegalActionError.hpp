#pragma once

#include <stdexcept>

namespace antwika::holdem
{

    /**
     * @brief Thrown by Table::apply() when an action breaks the rules of
     * no-limit betting: checking with a bet to call, raising below the
     * minimum with chips left behind, betting more than the stack holds,
     * and so on.
     *
     * Distinct from TableStateError: this one says the player's decision
     * was wrong, not that the table was asked something it could not
     * answer.
     */
    class IllegalActionError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::holdem
