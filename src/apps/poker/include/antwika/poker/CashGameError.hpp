#pragma once

#include <stdexcept>

namespace antwika::poker
{

    /**
     * @brief Thrown when a cash game cannot honour a request about who
     * sits where: a buy-in under the table minimum, a full table, a
     * cash-out by somebody who is not seated, or either one attempted in
     * the middle of a hand.
     *
     * Distinct from BankrollError, which is only ever about a player not
     * having the money.
     */
    class CashGameError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::poker
