#pragma once

#include <chrono>
#include <cstddef>

#include <antwika/time/IClock.hpp>

#include "antwika/app/FramePacing.hpp"

namespace antwika::app
{

    class TickDebt final
    {
    public:
        explicit TickDebt(const time::IClock &clock) noexcept;

        void start();

        [[nodiscard]] std::chrono::nanoseconds advance();

        [[nodiscard]] bool owesTick() const noexcept;

        void payTick() noexcept;

        void forgive() noexcept;

        [[nodiscard]] std::chrono::nanoseconds getOwedTime() const noexcept;

        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock>
        startedAt() const noexcept;

    private:
        const time::IClock *clock;
        std::chrono::time_point<std::chrono::system_clock> lastFrameAt;
        std::chrono::nanoseconds owedTime{};
    };

}
