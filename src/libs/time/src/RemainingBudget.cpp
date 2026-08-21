#include "antwika/time/RemainingBudget.hpp"

#include <chrono>

namespace antwika::time
{

    std::chrono::milliseconds remainingOf(
        const std::chrono::milliseconds budget,
        const std::chrono::nanoseconds spentTime)
    {
        if (spentTime.count() <= 0)
        {
            return budget;
        }

        if (spentTime >= budget)
        {
            return std::chrono::milliseconds::zero();
        }

        return std::chrono::ceil<std::chrono::milliseconds>(
            budget - spentTime);
    }

}
