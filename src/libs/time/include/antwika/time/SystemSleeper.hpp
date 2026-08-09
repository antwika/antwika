#pragma once

#include <chrono>

#include "antwika/time/ISleeper.hpp"

namespace antwika::time
{

    class SystemSleeper final : public ISleeper
    {
    public:
        void sleep(std::chrono::milliseconds duration) override;
    };

}
