#pragma once

#include <antwika/time/IClock.hpp>

namespace antwika::time {

    class TIME_EXPORT FakeClock : public IClock
    {
    public:
        explicit FakeClock(std::chrono::time_point<std::chrono::system_clock> t): current(t) {}

        std::chrono::time_point<std::chrono::system_clock> now() const override {
            return current;
        }

        void advance(std::chrono::seconds s) {
            current += s;
        }

        void set(std::chrono::time_point<std::chrono::system_clock> t) {
            current = t;
        }

    private:
        std::chrono::time_point<std::chrono::system_clock> current;
    };
}
