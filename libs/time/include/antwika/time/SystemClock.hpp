#pragma once

#include <chrono>

#include "antwika/time/IClock.hpp"

namespace antwika::time {

    class TIME_EXPORT SystemClock : public IClock
    {
    public:
        std::chrono::time_point<std::chrono::system_clock> now() const override;
    };
}
