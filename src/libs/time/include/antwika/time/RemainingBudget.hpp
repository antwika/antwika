#pragma once

#include <chrono>

namespace antwika::time
{

    [[nodiscard]] std::chrono::milliseconds remainingOf(
        std::chrono::milliseconds budget,
        std::chrono::nanoseconds spentTime);

}
