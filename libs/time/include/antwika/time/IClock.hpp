#pragma once

#include <chrono>

#ifdef _WIN32
#define TIME_EXPORT __declspec(dllexport)
#else
#define TIME_EXPORT
#endif

namespace antwika::time
{

    class TIME_EXPORT IClock
    {
    public:
        virtual ~IClock() = default;
        virtual std::chrono::time_point<std::chrono::system_clock> now() const = 0;
    };
} // namespace antwika::time
