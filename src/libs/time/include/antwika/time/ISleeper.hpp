#pragma once

#include <chrono>

namespace antwika::time
{

    class ISleeper
    {
    public:
        virtual ~ISleeper() = default;

        virtual void sleep(std::chrono::milliseconds duration) = 0;
    };

}
