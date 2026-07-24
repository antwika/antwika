#pragma once

#include <chrono>

#include "antwika/time/IClock.hpp"

namespace antwika::time
{

    class SystemClock : public IClock
    {
    public:
        std::chrono::time_point<std::chrono::system_clock> now() const override;
    };
}
