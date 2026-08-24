#pragma once

#include <cstdint>

namespace antwika::editor
{

    struct RemeshDebt final
    {
        std::uint32_t lastWheelNudgeTick = 0;

        bool afterNudge = false;

        bool pending = false;
    };

}
