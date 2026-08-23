#include "antwika/app/TickDebt.hpp"

namespace antwika::app
{

    TickDebt::TickDebt(const time::IClock &clock) noexcept
        : clock(&clock), lastFrameAt(clock.now())
    {
    }

    void TickDebt::start()
    {
        lastFrameAt = clock->now();
        owedTime = std::chrono::nanoseconds{};
    }

    std::chrono::nanoseconds TickDebt::advance()
    {
        const auto startedAt = clock->now();
        const auto since =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                startedAt - lastFrameAt);

        lastFrameAt = startedAt;
        owedTime += since;

        return since;
    }

    bool TickDebt::owesTick() const noexcept
    {
        return owedTime >= kTickPeriod;
    }

    void TickDebt::payTick() noexcept
    {
        owedTime -= kTickPeriod;
    }

    void TickDebt::forgive() noexcept
    {
        owedTime = std::chrono::nanoseconds{};
    }

    std::chrono::nanoseconds TickDebt::owed() const noexcept
    {
        return owedTime;
    }

    std::chrono::time_point<std::chrono::system_clock>
    TickDebt::startedAt() const noexcept
    {
        return lastFrameAt;
    }

}
