#pragma once

#include <stdexcept>

namespace antwika::poker
{

    /**
     * @brief Thrown by PokerRoomSink when an event's payload is not
     * valid JSON, is missing a field, or has one of the wrong type or
     * out of range.
     *
     * About the shape of the input, not about the poker: a payload that
     * parses but asks for something impossible raises BankrollError or
     * CashGameError instead.
     */
    class PokerEventError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::poker
