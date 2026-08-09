#pragma once

#include <cstdint>

namespace antwika::sudoku
{

    enum class MessageId : std::uint16_t
    {
        Title,

        SolveButton,

        Hint,

        Solved,

        Complete,

        NoSolution,

        LimitExceeded,

        GivenLocked,

        Count,
    };

}
