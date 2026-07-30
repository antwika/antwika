#pragma once

#include <stdexcept>

namespace antwika::holdem
{

    /**
     * @brief Thrown when a Table is asked to do something its current
     * state has no answer for: seating into an occupied or out-of-range
     * seat, starting a hand while one is running or with fewer than two
     * funded players, acting when nobody is to act, or reading the
     * result of a hand that has not finished.
     *
     * Distinct from IllegalActionError, which is about a player breaking
     * the betting rules rather than a caller misusing the table.
     */
    class TableStateError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::holdem
