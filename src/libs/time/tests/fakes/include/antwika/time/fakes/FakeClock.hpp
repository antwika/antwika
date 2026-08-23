#pragma once

#include <antwika/time/IClock.hpp>

namespace antwika::time::fakes
{

    using antwika::time::IClock;

    class FakeClock final : public IClock
    {
    public:
        explicit FakeClock(std::chrono::time_point<std::chrono::system_clock> t)
            : current(t)
        {
        }

        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock>
        currentTime() const noexcept override
        {
            return current;
        }

        void advance(std::chrono::milliseconds step)
        {
            current += step;
        }

        void set(std::chrono::time_point<std::chrono::system_clock> t)
        {
            current = t;
        }

    private:
        std::chrono::time_point<std::chrono::system_clock> current;
    };
}
