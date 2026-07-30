#pragma once

#include <stdexcept>

namespace antwika::poker
{

    /**
     * @brief Thrown when a withdrawal would take a player's bankroll
     * below zero.
     *
     * The one rule the ledger exists to enforce: a player can lose what
     * they brought to a table and no more, so no seat, no pot and no
     * buy-in may ever conjure chips a bankroll does not hold.
     */
    class BankrollError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::poker
